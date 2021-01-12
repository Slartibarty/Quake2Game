//=================================================================================================
// WinSock2 implementation of Quake's networking layer
//=================================================================================================

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include "../../common/q_shared.h"

#include "common.h"
#include "cvar.h"
#include "protocol.h"

#include "net.h"

#define	MAX_LOOPBACK	4

struct loopmsg_t
{
	byte	data[MAX_MSGLEN];
	int		datalen;
};

struct loopback_t
{
	loopmsg_t	msgs[MAX_LOOPBACK];
	int			get, send;
};

static cvar_t	*noudp;

loopback_t	loopbacks[2];
SOCKET		ip_sockets[2];

const char *NET_ErrorString (void);

//=============================================================================

static void NetadrToSockadr( const netadr_t *a, sockaddr *s )
{
	// Never loopback
	assert( a->type == NA_BROADCAST || a->type == NA_IP );

	memset( s, 0, sizeof( *s ) );

	switch ( a->type )
	{
	case NA_BROADCAST:
		( (sockaddr_in *)s )->sin_family = AF_INET;
		( (sockaddr_in *)s )->sin_port = a->port;
		( (sockaddr_in *)s )->sin_addr.s_addr = INADDR_BROADCAST;
		break;
	case NA_IP:
		( (sockaddr_in *)s )->sin_family = AF_INET;
		( (sockaddr_in *)s )->sin_addr.s_addr = *(ULONG *)&a->ip;
		( (sockaddr_in *)s )->sin_port = a->port;
		break;
	}
}

static void SockadrToNetadr( const sockaddr *s, netadr_t *a )
{
	if ( s->sa_family == AF_INET )
	{
		a->type = NA_IP;
		*(ULONG *)&a->ip = ( (sockaddr_in *)s )->sin_addr.s_addr;
		a->port = ( (sockaddr_in *)s )->sin_port;
	}
}

/*
===================
NET_CompareAdr
===================
*/
bool NET_CompareAdr( const netadr_t &a, const netadr_t &b )
{
	if ( a.type != b.type ) {
		return false;
	}

	if ( a.type == NA_LOOPBACK ) {
		return true;
	}

	if ( a.type == NA_IP ) {
		if ( a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port ) {
			return true;
		}
		return false;
	}

	return false;
}

/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
bool NET_CompareBaseAdr( const netadr_t &a, const netadr_t &b )
{
	if ( a.type != b.type ) {
		return false;
	}

	if ( a.type == NA_LOOPBACK ) {
		return true;
	}

	if ( a.type == NA_IP ) {
		if ( a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] ) {
			return true;
		}
		return false;
	}

	return false;
}

char *NET_AdrToString( const netadr_t &a )
{
	static char s[64];

	// Never broadcast
	assert( a.type == NA_LOOPBACK || a.type == NA_IP );

	switch ( a.type )
	{
	case NA_LOOPBACK:
		strcpy( s, "loopback" );
		break;
	case NA_IP:
		Com_sprintf( s, "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs( a.port ) );
		break;
	}

	return s;
}


/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
static bool NET_StringToSockaddr( const char *s, sockaddr *sadr )
{
	char	copy[128];

	memset( sadr, 0, sizeof( *sadr ) );

	if ( ( strlen( s ) >= 23 ) && ( s[8] == ':' ) && ( s[21] == ':' ) )	// check for an IPX address
	{
		// IPX crap was here
	}
	else
	{
		( (sockaddr_in *)sadr )->sin_family = AF_INET;

		( (sockaddr_in *)sadr )->sin_port = 0;

		Q_strcpy( copy, s );
		// strip off a trailing :port if present
		for ( char *colon = copy; *colon; ++colon )
		{
			if ( *colon == ':' )
			{
				*colon = '\0';
				( (sockaddr_in *)sadr )->sin_port = htons( (USHORT)atoi( colon + 1 ) );
			}
		}

		if ( copy[0] >= '0' && copy[0] <= '9' )
		{
			*(unsigned long *)&( (sockaddr_in *)sadr )->sin_addr = inet_addr( copy );
		}
		else
		{
			hostent *h = gethostbyname( copy );
			if ( !h ) {
				return false;
			}

			( (sockaddr_in *)sadr )->sin_addr = *(IN_ADDR *)h->h_addr_list[0];
		}
	}

	return true;
}

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
bool NET_StringToAdr( const char *s, netadr_t &a )
{
	sockaddr sadr;

	if ( strcmp( s, "localhost" ) == 0 )
	{
		memset( &a, 0, sizeof( a ) );
		a.type = NA_LOOPBACK;
		return true;
	}

	if ( !NET_StringToSockaddr( s, &sadr ) ) {
		return false;
	}

	SockadrToNetadr( &sadr, &a );

	return true;
}


bool NET_IsLocalAddress( const netadr_t &adr )
{
	return adr.type == NA_LOOPBACK;
}

/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/

static bool NET_GetLoopPacket( netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message )
{
	loopback_t *loop;

	loop = &loopbacks[sock];

	if ( loop->send - loop->get > MAX_LOOPBACK ) {
		loop->get = loop->send - MAX_LOOPBACK;
	}

	if ( loop->get >= loop->send ) {
		return false;
	}

	int i = loop->get & ( MAX_LOOPBACK - 1 );
	loop->get++;

	memcpy( net_message->data, loop->msgs[i].data, loop->msgs[i].datalen );
	net_message->cursize = loop->msgs[i].datalen;
	memset( net_from, 0, sizeof( *net_from ) );
	net_from->type = NA_LOOPBACK;

	return true;
}


static void NET_SendLoopPacket( netsrc_t sock, int length, void *data )
{
	int		i;
	loopback_t *loop;

	loop = &loopbacks[sock ^ 1];

	i = loop->send & ( MAX_LOOPBACK - 1 );
	loop->send++;

	memcpy( loop->msgs[i].data, data, length );
	loop->msgs[i].datalen = length;
}

//=============================================================================

bool NET_GetPacket( netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message )
{
	int 	ret;
	sockaddr from;
	int		fromlen;
	SOCKET	net_socket;
	int		protocol;
	int		err;

	if ( NET_GetLoopPacket( sock, net_from, net_message ) ) {
		return true;
	}

	for ( protocol = 0; protocol < 2; ++protocol )
	{
		if ( protocol == 0 ) {
			net_socket = ip_sockets[sock];
		}
		/*else {
			net_socket = ipx_sockets[sock];
		}*/

		if ( !net_socket ) {
			continue;
		}

		fromlen = sizeof( from );
		ret = recvfrom( net_socket, (char *)net_message->data, net_message->maxsize, 0, &from, &fromlen );

		SockadrToNetadr( &from, net_from );

		if ( ret == -1 )
		{
			err = WSAGetLastError();

			if ( err == WSAEWOULDBLOCK ) {
				continue;
			}
			if ( err == WSAEMSGSIZE ) {
				Com_Printf( "Warning: Oversize packet from %s\n", NET_AdrToString( *net_from ) );
				continue;
			}

			if ( dedicated->value ) {	// let dedicated servers continue after errors
				Com_Printf( "NET_GetPacket: %s from %s\n", NET_ErrorString(), NET_AdrToString( *net_from ) );
			}
			else {
				Com_Error( ERR_DROP, "NET_GetPacket: %s from %s", NET_ErrorString(), NET_AdrToString( *net_from ) );
			}
			continue;
		}

		if ( ret == net_message->maxsize )
		{
			Com_Printf( "Oversize packet from %s\n", NET_AdrToString( *net_from ) );
			continue;
		}

		net_message->cursize = ret;
		return true;
	}

	return false;
}

//=============================================================================

void NET_SendPacket( netsrc_t sock, int length, void *data, const netadr_t &to )
{
	int			ret;
	sockaddr	addr;
	SOCKET		net_socket;
	int			err;

	if ( to.type == NA_LOOPBACK )
	{
		NET_SendLoopPacket( sock, length, data );
		return;
	}

	switch ( to.type )
	{
	case NA_BROADCAST:
	case NA_IP:
		net_socket = ip_sockets[sock];
		if ( !net_socket ) {
			return;
		}
		break;
	default:
		Com_Error( ERR_FATAL, "NET_SendPacket: bad address type" );
	}

	NetadrToSockadr( &to, &addr );

	ret = sendto( net_socket, (char *)data, length, 0, &addr, sizeof( addr ) );
	if ( ret == -1 )
	{
		err = WSAGetLastError();

		// wouldblock is silent
		if ( err == WSAEWOULDBLOCK ) {
			return;
		}

		// some PPP links dont allow broadcasts
		if ( err == WSAEADDRNOTAVAIL && to.type == NA_BROADCAST ) {
			return;
		}

		if ( dedicated->value ) // let dedicated servers continue after errors
		{
			Com_Printf( "NET_SendPacket ERROR: %s to %s\n", NET_ErrorString(), NET_AdrToString( to ) );
		}
		else
		{
			if ( err == WSAEADDRNOTAVAIL ) {
				Com_DPrintf( "NET_SendPacket Warning: %s : %s\n", NET_ErrorString(), NET_AdrToString( to ) );
			}
			else {
				Com_Error( ERR_DROP, "NET_SendPacket ERROR: %s to %s\n", NET_ErrorString(), NET_AdrToString( to ) );
			}
		}
	}
}


//=============================================================================


/*
====================
NET_Socket
====================
*/
static SOCKET NET_IPSocket( const char *net_interface, int port )
{
	SOCKET			newsocket;
	sockaddr_in		address;
	ULONG			_true = 1;
	int				i = 1;
	int				err;

	if ( ( newsocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == -1 )
	{
		err = WSAGetLastError();
		if ( err != WSAEAFNOSUPPORT ) {
			Com_Printf( "WARNING: UDP_OpenSocket: socket: %s", NET_ErrorString() );
		}
		return 0;
	}

	// make it non-blocking
	if ( ioctlsocket( newsocket, FIONBIO, &_true ) == -1 )
	{
		Com_Printf( "WARNING: UDP_OpenSocket: ioctl FIONBIO: %s\n", NET_ErrorString() );
		return 0;
	}

	// make it broadcast capable
	if ( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof( i ) ) == -1 )
	{
		Com_Printf( "WARNING: UDP_OpenSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString() );
		return 0;
	}

	if ( !net_interface || !net_interface[0] || !Q_stricmp( net_interface, "localhost" ) ) {
		address.sin_addr.s_addr = INADDR_ANY;
	}
	else {
		NET_StringToSockaddr( net_interface, (sockaddr *)&address );
	}

	if ( port == PORT_ANY ) {
		address.sin_port = 0;
	}
	else {
		address.sin_port = htons( (USHORT)port );
	}

	address.sin_family = AF_INET;

	if ( bind( newsocket, (sockaddr *)&address, sizeof( address ) ) == -1 )
	{
		Com_Printf( "WARNING: UDP_OpenSocket: bind: %s\n", NET_ErrorString() );
		closesocket( newsocket );
		return 0;
	}

	return newsocket;
}


/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP( void )
{
	cvar_t *ip;
	int		port;
	int		dedicated;

	ip = Cvar_Get( "ip", "localhost", CVAR_NOSET );

	dedicated = (int)Cvar_VariableValue( "dedicated" );

	if ( !ip_sockets[NS_SERVER] )
	{
		port = Cvar_Get( "ip_hostport", "0", CVAR_NOSET )->value;
		if ( !port )
		{
			port = Cvar_Get( "hostport", "0", CVAR_NOSET )->value;
			if ( !port )
			{
				port = Cvar_Get( "port", XSTRINGIFY( PORT_SERVER ), CVAR_NOSET )->value;
			}
		}
		ip_sockets[NS_SERVER] = NET_IPSocket( ip->string, port );
		if ( !ip_sockets[NS_SERVER] && dedicated ) {
			Com_Error( ERR_FATAL, "Couldn't allocate dedicated server IP port" );
		}
	}

	// dedicated servers don't need client ports
	if ( dedicated )
		return;

	if ( !ip_sockets[NS_CLIENT] )
	{
		port = Cvar_Get( "ip_clientport", "0", CVAR_NOSET )->value;
		if ( !port )
		{
			port = Cvar_Get( "clientport", va( "%i", PORT_CLIENT ), CVAR_NOSET )->value;
			if ( !port ) {
				port = PORT_ANY;
			}
		}
		ip_sockets[NS_CLIENT] = NET_IPSocket( ip->string, port );
		if ( !ip_sockets[NS_CLIENT] ) {
			ip_sockets[NS_CLIENT] = NET_IPSocket( ip->string, PORT_ANY );
		}
	}
}


/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void NET_Config( bool multiplayer )
{
	int i;
	static bool old_config;

	if ( old_config == multiplayer ) {
		return;
	}

	old_config = multiplayer;

	if ( !multiplayer )
	{	// shut down any existing sockets
		for ( i = 0; i < 2; ++i )
		{
			if ( ip_sockets[i] )
			{
				closesocket( ip_sockets[i] );
				ip_sockets[i] = 0;
			}
		}
	}
	else
	{	// open sockets
		if ( !noudp->value )
			NET_OpenIP();
	}
}

// sleeps msec or until net socket is ready
void NET_Sleep( int msec )
{
	timeval timeout;
	fd_set	fdset;
	int i;

	if ( !dedicated || !dedicated->value ) {
		return; // we're not a server, just run full speed
	}

	FD_ZERO( &fdset );
	i = 0;
	if ( ip_sockets[NS_SERVER] ) {
		FD_SET( ip_sockets[NS_SERVER], &fdset ); // network socket
		i = ip_sockets[NS_SERVER];
	}
	timeout.tv_sec = msec / 1000;
	timeout.tv_usec = ( msec % 1000 ) * 1000;
	select( i + 1, &fdset, NULL, NULL, &timeout );
}

//===================================================================

/*
====================
NET_Init
====================
*/
void NET_Init( void )
{
	constexpr WORD versionRequested = MAKEWORD( 2, 2 );
	WSADATA wsadata;

	int err = WSAStartup( versionRequested, &wsadata );

	if ( err != 0 ) {
		Com_Error( ERR_FATAL, "Winsock initialization failed." );
	}

	Com_Printf( "Winsock Initialized\n" );

	noudp = Cvar_Get( "noudp", "0", CVAR_NOSET );
}


/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown( void )
{
	NET_Config( false );	// close sockets

	WSACleanup();
}

/*
====================
NET_ErrorString
====================
*/
const char *NET_ErrorString (void)
{
	int		code;

	code = WSAGetLastError ();
	switch (code)
	{
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	default: return "NO ERROR";
	}
}
