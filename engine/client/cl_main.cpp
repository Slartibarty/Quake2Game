// cl_main.c  -- client main loop

#include "cl_local.h"

StaticCvar cl_drawviewmodel( "cl_drawviewmodel", "1", 0 );
StaticCvar cl_footsteps( "cl_footsteps", "1", 0 );
StaticCvar cl_noskins( "cl_noskins", "0", 0 );
StaticCvar cl_autoskins( "cl_autoskins", "0", 0 );
StaticCvar cl_predict( "cl_predict", "1", 0 );

//static StaticCvar cl_minfps( "cl_minfps", "5", 0 );
static StaticCvar cl_maxfps( "cl_maxfps", "90", 0 );

StaticCvar cl_upspeed( "cl_upspeed", "175", 0 );
StaticCvar cl_forwardspeed( "cl_forwardspeed", "175", 0 );
StaticCvar cl_sidespeed( "cl_sidespeed", "175", 0 );
StaticCvar cl_yawspeed( "cl_yawspeed", "140", 0 );
StaticCvar cl_pitchspeed( "cl_pitchspeed", "150", 0 );
StaticCvar cl_anglespeedkey( "cl_anglespeedkey", "1.5", 0 );

StaticCvar cl_run( "cl_run", "0", CVAR_ARCHIVE );
StaticCvar sensitivity( "sensitivity", "3", CVAR_ARCHIVE );

StaticCvar m_pitch( "m_pitch", "0.022", CVAR_ARCHIVE );
StaticCvar m_yaw( "m_yaw", "0.022", 0 );
StaticCvar m_forward( "m_forward", "1", 0 );
StaticCvar m_side( "m_side", "1", 0 );

StaticCvar cl_shownet( "cl_shownet", "0", 0 );
StaticCvar cl_showmiss( "cl_showmiss", "0", 0 );
StaticCvar cl_showclamp( "showclamp", "0", 0 );
StaticCvar cl_paused( "paused", "0", 0 );
StaticCvar cl_timedemo( "timedemo", "0", 0 );
static StaticCvar cl_timeout( "cl_timeout", "120", 0 );

static StaticCvar rcon_client_password( "rcon_password", "", 0 );
static StaticCvar rcon_address( "rcon_address", "", 0 );

StaticCvar cl_lightlevel( "r_lightlevel", "0", 0 );

//
// userinfo
//
static StaticCvar info_password( "password", "", CVAR_USERINFO );
static StaticCvar info_spectator( "spectator", "0", CVAR_USERINFO );
	   StaticCvar name( "name", "unnamed", CVAR_USERINFO | CVAR_ARCHIVE );
	   StaticCvar skin( "skin", "male/grunt", CVAR_USERINFO | CVAR_ARCHIVE );
	   StaticCvar rate( "rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE );	// FIXME
static StaticCvar msg( "msg", "1", CVAR_USERINFO | CVAR_ARCHIVE );
	   StaticCvar hand( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
static StaticCvar fov( "fov", "90", CVAR_USERINFO | CVAR_ARCHIVE );
static StaticCvar gender( "gender", "male", CVAR_USERINFO | CVAR_ARCHIVE );
static StaticCvar gender_auto( "gender_auto", "1", CVAR_ARCHIVE );

StaticCvar cl_vwep( "cl_vwep", "1", CVAR_ARCHIVE );

clientStatic_t	cls;
clientActive_t	cl;

centity_t		cl_entities[MAX_EDICTS];

entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];

extern cvar_t *allow_download;
extern cvar_t *allow_download_players;
extern cvar_t *allow_download_models;
extern cvar_t *allow_download_sounds;
extern cvar_t *allow_download_maps;

static void CL_Disconnect();

//=============================================================================

/*
========================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
========================
*/
void CL_WriteDemoMessage()
{
	// the first eight bytes are just packet sequencing stuff
	int len = net_message.cursize - 8;
	int swlen = LittleLong( len );
	FileSystem::WriteFile( &swlen, sizeof( swlen ), cls.demofile );
	FileSystem::WriteFile( net_message.data + 8, len, cls.demofile );
}

/*
========================
CL_Stop_f

stop recording a demo
========================
*/
static void CL_Stop_f()
{
	if ( !cls.demorecording )
	{
		Com_Print( "Not recording a demo.\n" );
		return;
	}

	// finish up
	int len = -1;
	FileSystem::WriteFile( &len, sizeof( len ), cls.demofile );
	FileSystem::CloseFile( cls.demofile );
	cls.demofile = FS_INVALID_HANDLE;
	cls.demorecording = false;
	Com_Print( "Stopped demo.\n" );
}

/*
========================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
========================
*/
static void CL_Record_f()
{
	byte	buf_data[MAX_MSGLEN];
	sizebuf_t	buf;
	int		i;
	int		len;
	entity_state_t	*ent;
	entity_state_t	nullstate;

	if ( Cmd_Argc() != 2 )
	{
		Com_Print( "Usage: record <demoname>\n" );
		return;
	}

	if ( cls.demorecording )
	{
		Com_Print( "Already recording.\n" );
		return;
	}

	if ( cls.state != ca_active )
	{
		Com_Print( "You must be in a level to record.\n" );
		return;
	}

	//
	// open the demo file
	//
	const char *demoName = Cmd_Argv( 1 );
	Com_Printf( "recording to %s.\n", demoName );
	cls.demofile = FileSystem::OpenFileWrite( demoName );
	if ( cls.demofile == FS_INVALID_HANDLE )
	{
		Com_Print( S_COLOR_RED "ERROR: couldn't open.\n" );
		return;
	}
	cls.demorecording = true;

	// don't start saving messages until a non-delta compressed message is received
	cls.demowaiting = true;

	//
	// write out messages to hold the startup information
	//
	SZ_Init( &buf, buf_data, sizeof( buf_data ) );

	// send the serverdata
	MSG_WriteByte( &buf, svc_serverdata );
	MSG_WriteLong( &buf, PROTOCOL_VERSION );
	MSG_WriteLong( &buf, 0x10000 + cl.servercount );
	MSG_WriteByte( &buf, 1 );	// demos are always attract loops
	MSG_WriteString( &buf, "WackassNutty" );
	MSG_WriteShort( &buf, cl.playernum );

	MSG_WriteString( &buf, cl.configstrings[CS_NAME] );

	// Write configstrings
	for ( i = 0; i < MAX_CONFIGSTRINGS; i++ )
	{
		if ( cl.configstrings[i][0] )
		{
			if ( buf.cursize + static_cast<int>( strlen( cl.configstrings[i] ) + 32 ) > buf.maxsize )
			{
				// write it out
				len = LittleLong( buf.cursize );
				FileSystem::WriteFile( &len, sizeof( len ), cls.demofile );
				FileSystem::WriteFile( buf.data, buf.cursize, cls.demofile );
				buf.cursize = 0;
			}

			MSG_WriteByte( &buf, svc_configstring );
			MSG_WriteShort( &buf, i );
			MSG_WriteString( &buf, cl.configstrings[i] );
		}
	}

	// Write baselines
	memset( &nullstate, 0, sizeof( nullstate ) );
	for ( i = 0; i < MAX_EDICTS; i++ )
	{
		ent = &cl_entities[i].baseline;
		if ( !ent->modelindex )
		{
			continue;
		}

		if ( buf.cursize + 64 > buf.maxsize )
		{
			// write it out
			len = LittleLong( buf.cursize );
			FileSystem::WriteFile( &len, sizeof( len ), cls.demofile );
			FileSystem::WriteFile( buf.data, buf.cursize, cls.demofile );
			buf.cursize = 0;
		}

		MSG_WriteByte( &buf, svc_spawnbaseline );
		MSG_WriteDeltaEntity( &nullstate, &cl_entities[i].baseline, &buf, true, true );
	}

	MSG_WriteByte( &buf, svc_stufftext );
	MSG_WriteString( &buf, "precache\n" );

	// Write it to the demo file

	len = LittleLong( buf.cursize );
	FileSystem::WriteFile( &len, sizeof( len ), cls.demofile );
	FileSystem::WriteFile( buf.data, buf.cursize, cls.demofile );

	// the rest of the demo file will be individual frames
}

//=================================================================================================

/*
========================
Cmd_ForwardToServer

adds the current command line as a clc_stringcmd to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
========================
*/
void Cmd_ForwardToServer()
{
	const char *cmd = Cmd_Argv( 0 );
	if ( cls.state <= ca_connected || *cmd == '-' || *cmd == '+' )
	{
		Com_Printf( "Unknown command \"%s\"\n", cmd );
		return;
	}

	MSG_WriteByte( &cls.netchan.message, clc_stringcmd );
	SZ_Print( &cls.netchan.message, cmd );
	if ( Cmd_Argc() > 1 )
	{
		SZ_Print( &cls.netchan.message, " " );
		SZ_Print( &cls.netchan.message, Cmd_Args() );
	}
}

/*
=======================
CL_ForwardToServer_f
=======================
*/
static void CL_ForwardToServer_f()
{
	if ( cls.state != ca_connected && cls.state != ca_active )
	{
		Com_Printf( "Can't \"%s\", not connected\n", Cmd_Argv( 0 ) );
		return;
	}

	// don't forward the first argument
	if ( Cmd_Argc() > 1 )
	{
		MSG_WriteByte( &cls.netchan.message, clc_stringcmd );
		SZ_Print( &cls.netchan.message, Cmd_Args() );
	}
}

/*
=======================
CL_Pause_f
=======================
*/
void CL_Pause_f()
{
	// never pause in multiplayer
	if ( Cvar_FindGetFloat( "maxclients" ) > 1 || !Com_ServerState() )
	{
		Cvar_SetBool( &cl_paused, false );
		return;
	}

	Cvar_SetBool( &cl_paused, !cl_paused.GetBool() );
}

/*
=======================
CL_Quit_f
=======================
*/
void CL_Quit_f()
{
	CL_Disconnect();
	Com_Quit( EXIT_SUCCESS );
}

/*
=======================
CL_Drop

Called after an ERR_DROP was thrown
=======================
*/
void CL_Drop()
{
	if ( cls.state == ca_uninitialized || cls.state == ca_disconnected ) {
		return;
	}

	CL_Disconnect();

	// drop loading plaque unless this is the initial game start
	if ( cls.disable_servercount != -1 ) {
		// get rid of loading plaque
		SCR_EndLoadingPlaque();
	}
}

/*
=======================
CL_SendConnectPacket

We have gotten a challenge from the server, so try and
connect.
======================
*/
static void CL_SendConnectPacket()
{
	netadr_t adr;

	if ( !NET_StringToNetadr( cls.servername, adr ) )
	{
		Com_Print( "Bad server address\n" );
		cls.connect_time = 0;
		return;
	}
	if ( adr.port == 0 ) {
		adr.port = BigShort( PORT_SERVER );
	}

	int port = net_qport->GetInt();
	userinfo_modified = false;

	Netchan_OutOfBandPrint( NS_CLIENT, adr, "connect %i %i %i \"%s\"\n",
		PROTOCOL_VERSION, port, cls.challenge, Cvar_Userinfo() );
}

/*
========================
CL_CheckForResend

Resend a connect message if the last one has timed out
========================
*/
static void CL_CheckForResend()
{
	// if the local server is running and we aren't
	// then connect
	if ( cls.state == ca_disconnected && Com_ServerState() )
	{
		cls.state = ca_connecting;
		Q_strcpy_s( cls.servername, "localhost" );
		// we don't need a challenge on the localhost
		CL_SendConnectPacket();
		return;
		//cls.connect_time = -99999; // CL_CheckForResend() will fire immediately
	}

	// resend if we haven't gotten a reply yet
	if ( cls.state != ca_connecting ) {
		return;
	}

	if ( cls.realtime - cls.connect_time < 3000 ) {
		return;
	}

	netadr_t adr;

	if ( !NET_StringToNetadr( cls.servername, adr ) )
	{
		Com_Print( "Bad server address\n" );
		cls.state = ca_disconnected;
		return;
	}
	if ( adr.port == 0 ) {
		adr.port = BigShort( PORT_SERVER );
	}

	// for retransmit requests
	cls.connect_time = static_cast<float>( cls.realtime );

	Com_Printf( "Connecting to %s...\n", cls.servername );

	Netchan_OutOfBandPrint( NS_CLIENT, adr, "getchallenge\n" );
}

/*
========================
CL_Connect_f
========================
*/
static void CL_Connect_f()
{
	if ( Cmd_Argc() != 2 )
	{
		Com_Print( "usage: connect <server>\n" );
		return;
	}

	if ( Com_ServerState() )
	{
		AssertMsg( false, "You should break here and check to see what cvar_t *msg is set to." );

		// if running a local server, kill it and reissue
		//SV_Shutdown( va( "Server quit\n", msg ), false );
		SV_Shutdown( "Server quit\n", false );
	}
	else
	{
		CL_Disconnect();
	}

	const char *server = Cmd_Argv( 1 );

	// allow remote
	NET_Config( true );

	CL_Disconnect();

	cls.state = ca_connecting;
	Q_strcpy_s( cls.servername, server );
	cls.connect_time = -99999; // CL_CheckForResend() will fire immediately
}

/*
========================
CL_Rcon_f

Send the rest of the command line over as
an unconnected command.
========================
*/
static void CL_Rcon_f()
{
	if ( !rcon_client_password.GetString()[0] )
	{
		Com_Print(
			"You must set 'rcon_password' before\n"
			"issuing an rcon command.\n"
		);
		return;
	}

	char message[1024];

	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = 0;

	NET_Config( true );		// allow remote

	strcat( message, "rcon " );

	strcat( message, rcon_client_password.GetString() );
	strcat( message, " " );

	for ( int i = 1; i < Cmd_Argc(); ++i )
	{
		strcat( message, Cmd_Argv( i ) );
		strcat( message, " " );
	}

	netadr_t to;

	if ( cls.state >= ca_connected )
	{
		to = cls.netchan.remote_address;
	}
	else
	{
		if ( !strlen( rcon_address.GetString() ) )
		{
			Com_Print(
				"You must either be connected,\n"
				"or set the 'rcon_address' cvar\n"
				"to issue rcon commands\n"
			);
			return;
		}
		NET_StringToNetadr( rcon_address.GetString(), to );
		if ( to.port == 0 ) {
			to.port = BigShort( PORT_SERVER );
		}
	}

	NET_SendPacket( NS_CLIENT, (int)strlen( message ) + 1, message, to );
}

/*
========================
CL_ClearState
========================
*/
void CL_ClearState()
{
	S_StopAllSounds();

	cge->ClearState();

	// wipe the entire cl structure
	memset( &cl, 0, sizeof( cl ) );
	memset( &cl_entities, 0, sizeof( cl_entities ) );

	SZ_Clear( &cls.netchan.message );
}

/*
========================
CL_Disconnect

Goes from a connected state to full screen console state
Sends a disconnect message to the server
This is also called on Com_Error, so it shouldn't cause any errors
========================
*/
static void CL_Disconnect()
{
	char final[32];

	if ( cls.state == ca_disconnected ) {
		return;
	}

	if ( cl_timedemo.GetBool() )
	{
		int time = Sys_Milliseconds() - cl.timedemo_start;
		if ( time > 0 )
		{
			Com_Printf( "%i frames, %3.1f seconds: %3.1f fps\n",
				cl.timedemo_frames, time / 1000.0, cl.timedemo_frames * 1000.0 / time );
		}
	}

	VectorClear( cl.refdef.blend );
	R_SetRawPalette( NULL );

	M_ForceMenuOff();

	cls.connect_time = 0;

	SCR_StopCinematic();

	if ( cls.demorecording ) {
		CL_Stop_f();
	}

	// send a disconnect message to the server
	final[0] = clc_stringcmd;
	strcpy( (char *)final + 1, "disconnect" );
	int length = static_cast<int>( strlen( final ) );
	Netchan_Transmit( &cls.netchan, length, (byte *)final );
	Netchan_Transmit( &cls.netchan, length, (byte *)final );
	Netchan_Transmit( &cls.netchan, length, (byte *)final );

	CL_ClearState();

	// stop download
	if ( cls.download ) {
		FileSystem::CloseFile( cls.download );
		cls.download = NULL;
	}

	cls.state = ca_disconnected;
}

/*
========================
CL_Disconnect_f
========================
*/
static void CL_Disconnect_f()
{
	Com_Error( "Disconnected from server" );
}

/*
========================
CL_Packet_f

packet <destination> <contents>

Contents allows \n escape character
========================
*/
#ifdef Q_DEBUG
static void CL_Packet_f()
{
	if ( Cmd_Argc() != 3 )
	{
		Com_Printf( "packet <destination> <contents>\n" );
		return;
	}

	// allow remote
	NET_Config( true );

	netadr_t adr;

	if ( !NET_StringToNetadr( Cmd_Argv( 1 ), adr ) )
	{
		Com_Printf( "Bad address\n" );
		return;
	}
	if ( !adr.port ) {
		adr.port = BigShort( PORT_SERVER );
	}

	char send[2048];

	char *in = Cmd_Argv( 2 );
	char *out = send + 4;
	send[0] = send[1] = send[2] = send[3] = (char)0xff;

	const strlen_t length = Q_strlen( in );
	for ( strlen_t i = 0; i < length; ++i )
	{
		if ( in[i] == '\\' && in[i + 1] == 'n' )
		{
			*out++ = '\n';
			i++;
		}
		else
		{
			*out++ = in[i];
		}
	}
	*out = 0;

	NET_SendPacket( NS_CLIENT, static_cast<int>( out - send ), send, adr );
}
#endif

/*
========================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
========================
*/
void CL_Changing_f()
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if ( cls.download ) {
		return;
	}

	SCR_BeginLoadingPlaque();
	cls.state = ca_connected;	// not active anymore, but not disconnected
	Com_Print( "\nChanging map...\n" );
}

/*
========================
CL_Reconnect_f

The server is changing levels
========================
*/
static void CL_Reconnect_f()
{
	// ZOID
	// if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if ( cls.download ) {
		return;
	}

	S_StopAllSounds();
	if ( cls.state == ca_connected )
	{
		Com_Print( "reconnecting...\n" );
		cls.state = ca_connected;
		MSG_WriteChar( &cls.netchan.message, clc_stringcmd );
		MSG_WriteString( &cls.netchan.message, "new" );
		return;
	}

	if ( cls.servername[0] )
	{
		if ( cls.state >= ca_connected )
		{
			CL_Disconnect();
			cls.connect_time = (float)( cls.realtime - 1500 );
		}
		else
		{
			// fire immediately
			cls.connect_time = -99999;
		}

		cls.state = ca_connecting;
		Com_Print( "reconnecting...\n" );
	}
}

/*
========================
CL_ParseStatusMessage

Handle a reply from a ping
========================
*/
static void CL_ParseStatusMessage()
{
	char *s = MSG_ReadString( &net_message );

	Com_Printf( "%s\n", s );
	M_AddToServerList( net_from, s );
}

/*
========================
CL_PingServers_f
========================
*/
void CL_PingServers_f()
{
	int				i;
	netadr_t		adr;
	char			name[32];
	const char *	adrstring;
	cvar_t *		net_noudp;

	// allow remote
	NET_Config( true );

	// send a broadcast packet
	Com_Print( "pinging broadcast...\n" );

	net_noudp = Cvar_Get( "net_noudp", "0", CVAR_INIT );
	if ( !net_noudp->GetBool() )
	{
		adr.type = NA_BROADCAST;
		adr.port = BigShort( PORT_SERVER );
		Netchan_OutOfBandPrint( NS_CLIENT, adr, "info " STRINGIFY( PROTOCOL_VERSION ) );
	}

	// send a packet to each address book entry
	for ( i = 0; i < 16; ++i )
	{
		Q_sprintf_s( name, "adr%i", i );
		adrstring = Cvar_FindGetString( name );
		if ( !adrstring || !adrstring[0] ) {
			continue;
		}

		Com_Printf( "pinging %s...\n", adrstring );
		if ( !NET_StringToNetadr( adrstring, adr ) )
		{
			Com_Printf( "Bad address: %s\n", adrstring );
			continue;
		}
		if ( !adr.port ) {
			adr.port = BigShort( PORT_SERVER );
		}
		Netchan_OutOfBandPrint( NS_CLIENT, adr, "info " STRINGIFY( PROTOCOL_VERSION ) );
	}
}

/*
========================
CL_Skins_f

Load or download any custom player skins and models
========================
*/
static void CL_Skins_f()
{
	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( !cl.configstrings[CS_PLAYERSKINS + i][0] ) {
			continue;
		}

		Com_Printf( "client %i: %s\n", i, cl.configstrings[CS_PLAYERSKINS + i] );
		SCR_UpdateScreen();
		CL_ParseClientinfo( i );
	}
}

/*
========================
CL_ConnectionlessPacket

Responses to broadcasts, etc
========================
*/
static void CL_ConnectionlessPacket()
{
	char *s;
	char *c;

	MSG_BeginReading( &net_message );
	MSG_ReadLong( &net_message );	// skip the -1

	s = MSG_ReadStringLine( &net_message );

	Cmd_TokenizeString( s, false );

	c = Cmd_Argv( 0 );

	Com_Printf( "%s: %s\n", NET_NetadrToString( net_from ), c );

	// server connection
	if ( Q_strcmp( c, "client_connect" ) == 0 )
	{
		if ( cls.state == ca_connected )
		{
			Com_Printf( "Dup connect received.  Ignored.\n" );
			return;
		}
		Netchan_Setup( NS_CLIENT, &cls.netchan, net_from, cls.quakePort );
		MSG_WriteChar( &cls.netchan.message, clc_stringcmd );
		MSG_WriteString( &cls.netchan.message, "new" );
		cls.state = ca_connected;
		return;
	}

	// server responding to a status broadcast
	if ( Q_strcmp( c, "info" ) == 0 )
	{
		CL_ParseStatusMessage();
		return;
	}

	// remote command from gui front end
	if ( Q_strcmp( c, "cmd" ) == 0 )
	{
		if ( !NET_IsLocalAddress( net_from ) )
		{
			Com_Printf( "Command packet from remote host.  Ignored.\n" );
			return;
		}
		Sys_AppActivate();
		s = MSG_ReadString( &net_message );
		Cbuf_AddText( s );
		Cbuf_AddText( "\n" );
		return;
	}

	// print command from somewhere
	if ( Q_strcmp( c, "print" ) == 0 )
	{
		s = MSG_ReadString( &net_message );
		Com_Print( s );
		return;
	}

	// ping from somewhere
	if ( Q_strcmp( c, "ping" ) == 0 )
	{
		Netchan_OutOfBandPrint( NS_CLIENT, net_from, "ack" );
		return;
	}

	// challenge from the server we are connecting to
	if ( Q_strcmp( c, "challenge" ) == 0 )
	{
		cls.challenge = Q_atoi( Cmd_Argv( 1 ) );
		CL_SendConnectPacket();
		return;
	}

	// echo request from server
	if ( Q_strcmp( c, "echo" ) == 0 )
	{
		Netchan_OutOfBandPrint( NS_CLIENT, net_from, "%s", Cmd_Argv( 1 ) );
		return;
	}

	Com_Print( "Unknown command.\n" );
}

/*
========================
CL_DumpPackets

A vain attempt to help bad TCP stacks that cause problems
when they overflow
========================
*/
static void CL_DumpPackets()
{
	while ( NET_GetPacket( NS_CLIENT, &net_from, &net_message ) )
	{
		Com_Print( "dumnping a packet\n" );
	}
}

/*
========================
CL_ReadPackets
========================
*/
static void CL_ReadPackets()
{
	while ( NET_GetPacket( NS_CLIENT, &net_from, &net_message ) )
	{
		//Com_Print( "packet\n" );
		
		//
		// Remote command packet
		//
		if ( *(int *)net_message.data == -1 )
		{
			CL_ConnectionlessPacket();
			continue;
		}

		if ( cls.state == ca_disconnected || cls.state == ca_connecting ) {
			// Dump it if not connected
			continue;
		}

		if ( net_message.cursize < 8 )
		{
			Com_Printf( "%s: Runt packet\n", NET_NetadrToString( net_from ) );
			continue;
		}

		//
		// Packet from server
		//
		if ( !NET_CompareNetadr( net_from, cls.netchan.remote_address ) )
		{
			Com_DPrintf( "%s:sequenced packet without connection\n", NET_NetadrToString( net_from ) );
			continue;
		}

		if ( !Netchan_Process( &cls.netchan, &net_message ) ) {
			// wasn't accepted for some reason
			continue;
		}

		CL_ParseServerMessage();
	}

	//
	// check timeout
	//
	if ( cls.state >= ca_connected && cls.realtime - cls.netchan.last_received > SEC2MS( cl_timeout.GetInt() ) )
	{
		// Timeoutcount saves debugger
		if ( ++cl.timeoutcount > 5 )
		{
			Com_Printf( "\nServer connection timed out.\n" );
			CL_Disconnect();
			return;
		}
	}
	else
	{
		cl.timeoutcount = 0;
	}
}

//=================================================================================================

/*
========================
CL_FixUpGender_f
========================
*/
void CL_FixUpGender()
{
	if ( gender_auto.GetBool() ) {

		if ( gender.IsModified() ) {
			// was set directly, don't override the user
			gender.ClearModified();
			return;
		}

		char sk[80];
		Q_strcpy_s( sk, skin.GetString() );

		char *p;
		if ( ( p = strchr( sk, '/' ) ) != NULL ) {
			*p = 0;
		}

		if ( Q_stricmp( sk, "male" ) == 0 || Q_stricmp( sk, "cyborg" ) == 0 ) {
			Cvar_FindSetString( "gender", "male" );
		} else if ( Q_stricmp( sk, "female" ) == 0 || Q_stricmp( sk, "crackhor" ) == 0 ) {
			Cvar_FindSetString( "gender", "female" );
		} else {
			Cvar_FindSetString( "gender", "none" );
		}

		gender.ClearModified();
	}
}

/*
========================
CL_Userinfo_f
========================
*/
static void CL_Userinfo_f()
{
	Com_Printf( "User info settings:\n" );
	Info_Print( Cvar_Userinfo() );
}

/*
========================
CL_Snd_Restart_f

Restart the sound subsystem so it can pick up
new parameters and flush all sounds
========================
*/
void CL_Snd_Restart_f()
{
	S_Shutdown();
	S_Init();
	CL_RegisterSounds();
}

#if 0

int precache_check; // for autodownload of precache items
int precache_spawncount;
int precache_tex;
int precache_model_skin;

byte *precache_model; // used for skin checking in alias models

#define PLAYER_MULT 5

// ENV_CNT is map load, ENV_CNT+1 is first env map
#define ENV_CNT (CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
#define TEXTURE_CNT (ENV_CNT+13)

static const char *env_suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};

void CL_RequestNextDownload (void)
{
	unsigned	map_checksum;		// for detecting cheater maps
	char fn[MAX_OSPATH];
	dmdl_t *pheader;

	if (cls.state != ca_connected)
		return;

	if (!allow_download->GetBool() && precache_check < ENV_CNT)
		precache_check = ENV_CNT;

//ZOID
	if (precache_check == CS_MODELS) { // confirm map
		precache_check = CS_MODELS+2; // 0 isn't used
		if (allow_download_maps->GetBool())
			if (!CL_CheckOrDownloadFile(cl.configstrings[CS_MODELS+1]))
				return; // started a download
	}
	if (precache_check >= CS_MODELS && precache_check < CS_MODELS+MAX_MODELS) {
		if (allow_download_models->GetBool()) {
			while (precache_check < CS_MODELS+MAX_MODELS &&
				cl.configstrings[precache_check][0]) {
				if (cl.configstrings[precache_check][0] == '*' ||
					cl.configstrings[precache_check][0] == '#') {
					precache_check++;
					continue;
				}
				if (precache_model_skin == 0) {
					if (!CL_CheckOrDownloadFile(cl.configstrings[precache_check])) {
						precache_model_skin = 1;
						return; // started a download
					}
					precache_model_skin = 1;
				}

				// checking for skins in the model
				if (!precache_model) {

					FileSystem::LoadFile (cl.configstrings[precache_check], (void **)&precache_model);
					if (!precache_model) {
						precache_model_skin = 0;
						precache_check++;
						continue; // couldn't load it
					}
					if (LittleLong(*(unsigned *)precache_model) != IDALIASHEADER) {
						// not an alias model
						FileSystem::FreeFile(precache_model);
						precache_model = 0;
						precache_model_skin = 0;
						precache_check++;
						continue;
					}
					pheader = (dmdl_t *)precache_model;
					if (LittleLong (pheader->version) != ALIAS_VERSION) {
						precache_check++;
						precache_model_skin = 0;
						continue; // couldn't load it
					}
				}

				pheader = (dmdl_t *)precache_model;

				while (precache_model_skin - 1 < LittleLong(pheader->num_skins)) {
					if (!CL_CheckOrDownloadFile((char *)precache_model +
						LittleLong(pheader->ofs_skins) + 
						(precache_model_skin - 1)*MAX_SKINNAME)) {
						precache_model_skin++;
						return; // started a download
					}
					precache_model_skin++;
				}
				if (precache_model) { 
					FileSystem::FreeFile(precache_model);
					precache_model = 0;
				}
				precache_model_skin = 0;
				precache_check++;
			}
		}
		precache_check = CS_SOUNDS;
	}
	if (precache_check >= CS_SOUNDS && precache_check < CS_SOUNDS+MAX_SOUNDS) { 
		if (allow_download_sounds->GetBool()) {
			if (precache_check == CS_SOUNDS)
				precache_check++; // zero is blank
			while (precache_check < CS_SOUNDS+MAX_SOUNDS &&
				cl.configstrings[precache_check][0]) {
				if (cl.configstrings[precache_check][0] == '*') {
					precache_check++;
					continue;
				}
				Q_sprintf_s(fn, "sound/%s", cl.configstrings[precache_check++]);
				if (!CL_CheckOrDownloadFile(fn))
					return; // started a download
			}
		}
		precache_check = CS_IMAGES;
	}
	if (precache_check >= CS_IMAGES && precache_check < CS_IMAGES+MAX_IMAGES) {
		if (precache_check == CS_IMAGES)
			precache_check++; // zero is blank
		while (precache_check < CS_IMAGES+MAX_IMAGES &&
			cl.configstrings[precache_check][0]) {
			Q_sprintf_s(fn, "pics/%s.pcx", cl.configstrings[precache_check++]);
			if (!CL_CheckOrDownloadFile(fn))
				return; // started a download
		}
		precache_check = CS_PLAYERSKINS;
	}
	// skins are special, since a player has three things to download:
	// model, weapon model and skin
	// so precache_check is now *3
	if (precache_check >= CS_PLAYERSKINS && precache_check < CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT) {
		if (allow_download_players->GetBool()) {
			while (precache_check < CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT) {
				int i, n;
				char model[MAX_QPATH], skin[MAX_QPATH], *p;

				i = (precache_check - CS_PLAYERSKINS)/PLAYER_MULT;
				n = (precache_check - CS_PLAYERSKINS)%PLAYER_MULT;

				if (!cl.configstrings[CS_PLAYERSKINS+i][0]) {
					precache_check = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
					continue;
				}

				if ((p = strchr(cl.configstrings[CS_PLAYERSKINS+i], '\\')) != NULL)
					p++;
				else
					p = cl.configstrings[CS_PLAYERSKINS+i];
				strcpy(model, p);
				p = strchr(model, '/');
				if (!p)
					p = strchr(model, '\\');
				if (p) {
					*p++ = 0;
					strcpy(skin, p);
				} else
					*skin = 0;

				switch (n) {
				case 0: // model
					Q_sprintf_s(fn, "players/%s/tris.md2", model);
					if (!CL_CheckOrDownloadFile(fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 1;
						return; // started a download
					}
					n++;
					[[fallthrough]];

				case 1: // weapon model
					Q_sprintf_s(fn, "players/%s/weapon.md2", model);
					if (!CL_CheckOrDownloadFile(fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 2;
						return; // started a download
					}
					n++;
					[[fallthrough]];

				case 2: // weapon skin
					Q_sprintf_s(fn, "players/%s/weapon.pcx", model);
					if (!CL_CheckOrDownloadFile(fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 3;
						return; // started a download
					}
					n++;
					[[fallthrough]];

				case 3: // skin
					Q_sprintf_s(fn, "players/%s/%s.pcx", model, skin);
					if (!CL_CheckOrDownloadFile(fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 4;
						return; // started a download
					}
					n++;
					[[fallthrough]];

				case 4: // skin_i
					Q_sprintf_s(fn, "players/%s/%s_i.pcx", model, skin);
					if (!CL_CheckOrDownloadFile(fn)) {
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 5;
						return; // started a download
					}
					// move on to next model
					precache_check = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
				}
			}
		}
		// precache phase completed
		precache_check = ENV_CNT;
	}

	if (precache_check == ENV_CNT) {
		precache_check = ENV_CNT + 1;

		// SlartTodo: This is putrid
		CM_LoadMap (cl.configstrings[CS_MODELS+1], true, &map_checksum);

		if (map_checksum != Q_atoi(cl.configstrings[CS_MAPCHECKSUM])) {
			Com_Errorf ("Local map version differs from server: %i != '%s'\n",
				map_checksum, cl.configstrings[CS_MAPCHECKSUM]);
			return;
		}
	}

	if (precache_check > ENV_CNT && precache_check < TEXTURE_CNT) {
		if (allow_download->GetBool() && allow_download_maps->GetBool()) {
			while (precache_check < TEXTURE_CNT) {
				int n = precache_check++ - ENV_CNT - 1;

				if (n & 1)
					Q_sprintf_s(fn, "env/%s%s.pcx", 
						cl.configstrings[CS_SKY], env_suf[n/2]);
				else
					Q_sprintf_s(fn, "env/%s%s.tga", 
						cl.configstrings[CS_SKY], env_suf[n/2]);
				if (!CL_CheckOrDownloadFile(fn))
					return; // started a download
			}
		}
		precache_check = TEXTURE_CNT;
	}

	if (precache_check == TEXTURE_CNT) {
		precache_check = TEXTURE_CNT+1;
		precache_tex = 0;
	}

	// confirm existance of textures, download any that don't exist
	if (precache_check == TEXTURE_CNT+1) {
		// from qcommon/cmodel.c
#if 0 // Slart: This code blows
		extern int			numtexinfo;
		extern csurface_t	map_surfaces[];

		if (allow_download->value && allow_download_maps->value) {
			while (precache_tex < numtexinfo) {
				char fn[MAX_OSPATH];

				Q_sprintf_s(fn, "textures/%s.wal", map_surfaces[precache_tex++].name);
				if (!CL_CheckOrDownloadFile(fn))
					return; // started a download
			}
		}
#endif
		precache_check = TEXTURE_CNT+999;
	}

//ZOID
	CL_RegisterSounds ();
	CL_PrepRefresh ();

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, va("begin %i\n", precache_spawncount) );
}

#else

/*
======================
CL_RequestNextDownload
======================
*/
void CL_RequestNextDownload()
{
}

#endif

/*
========================
CL_Precache_f

The server will send this command right
before allowing the client into the server
========================
*/
static void CL_Precache_f()
{
#if 1
	uint mapChecksum; // for detecting cheater maps

	CM_LoadMap( cl.configstrings[CS_MODELS + 1], true, &mapChecksum );

	if ( mapChecksum != Q_atoi( cl.configstrings[CS_MAPCHECKSUM] ) )
	{
		Com_Errorf( "Local map version differs from server: %i != '%s'\n", mapChecksum, cl.configstrings[CS_MAPCHECKSUM] );
	}

	CL_RegisterSounds();
	CL_PrepRefresh();
#else
	//Yet another hack to let old demos work
	//the old precache sequence
	if (Cmd_Argc() < 2) {
		unsigned	map_checksum;		// for detecting cheater maps

		CM_LoadMap (cl.configstrings[CS_MODELS+1], true, &map_checksum);
		CL_RegisterSounds ();
		CL_PrepRefresh ();
		return;
	}

	precache_check = CS_MODELS;
	precache_spawncount = Q_atoi(Cmd_Argv(1));
	precache_model = 0;
	precache_model_skin = 0;

	CL_RequestNextDownload();
#endif
}

/*
========================
CL_InitLocal
========================
*/
static void CL_InitLocal()
{
	cls.state = ca_disconnected;
	cls.realtime = Sys_Milliseconds();

	CL_InitInput();

	// clear this so we know when user sets it manually
	gender.ClearModified();

	//
	// register our commands
	//
	Cmd_AddCommand( "cmd", CL_ForwardToServer_f );
	Cmd_AddCommand( "pause", CL_Pause_f );
	Cmd_AddCommand( "pingservers", CL_PingServers_f );
	Cmd_AddCommand( "skins", CL_Skins_f );

	Cmd_AddCommand( "userinfo", CL_Userinfo_f );
	Cmd_AddCommand( "snd_restart", CL_Snd_Restart_f );

	Cmd_AddCommand( "changing", CL_Changing_f );
	Cmd_AddCommand( "disconnect", CL_Disconnect_f );
	Cmd_AddCommand( "record", CL_Record_f );
	Cmd_AddCommand( "stop", CL_Stop_f );

	Cmd_AddCommand( "quit", CL_Quit_f );

	Cmd_AddCommand( "connect", CL_Connect_f );
	Cmd_AddCommand( "reconnect", CL_Reconnect_f );

	Cmd_AddCommand( "rcon", CL_Rcon_f );

#ifdef Q_DEBUG
	Cmd_AddCommand( "packet", CL_Packet_f ); // this is dangerous to leave in
#endif

	Cmd_AddCommand( "precache", CL_Precache_f );

	Cmd_AddCommand( "download", CL_Download_f );

	//
	// forward to server commands
	//
	// the only thing this does is allow command completion
	// to work -- all unknown commands are automatically
	// forwarded to the server
	// TODO: This sucks
	Cmd_AddCommand( "wave", NULL );
	Cmd_AddCommand( "inven", NULL );
	Cmd_AddCommand( "kill", NULL );
	Cmd_AddCommand( "use", NULL );
	Cmd_AddCommand( "drop", NULL );
	Cmd_AddCommand( "say", NULL );
	Cmd_AddCommand( "say_team", NULL );
	Cmd_AddCommand( "info", NULL );
	Cmd_AddCommand( "give", NULL );
	Cmd_AddCommand( "god", NULL );
	Cmd_AddCommand( "notarget", NULL );
	Cmd_AddCommand( "noclip", NULL );
	Cmd_AddCommand( "invuse", NULL );
	Cmd_AddCommand( "invprev", NULL );
	Cmd_AddCommand( "invnext", NULL );
	Cmd_AddCommand( "invdrop", NULL );
	Cmd_AddCommand( "weapnext", NULL );
	Cmd_AddCommand( "weapprev", NULL );
}

/*
========================
CL_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
========================
*/
void CL_WriteConfiguration()
{
	if ( cls.state == ca_uninitialized ) {
		return;
	}

	fsHandle_t handle = FileSystem::OpenFileWrite( "config.cfg" );
	if ( handle == FS_INVALID_HANDLE )
	{
		Com_Print( "Couldn't write config.cfg.\n" );
		return;
	}

	FileSystem::PrintFileFmt( handle, "// Generated by %s\n", FileSystem::ModInfo::GetGameTitle() );

	Key_WriteBindings( handle );
	Cvar_WriteVariables( handle );

	FileSystem::CloseFile( handle );
}

/*
========================
CL_FixCvarCheats
========================
*/

typedef struct
{
	const char	*name;
	const char	*value;
	cvar_t		*var;
} cheatvar_t;

cheatvar_t	cheatvars[]{
	{"com_timeScale", "1"},
	{"timedemo", "0"},
	{"r_drawworld", "1"},
	{"v_testLights", "0"},
	{"r_fullbright", "0"},
	{"paused", "0"},
	{"com_fixedTime", "0"},
	{"gl_lightmap", "0"},
	{NULL, NULL}
};

int		numcheatvars;

void CL_FixCvarCheats (void)
{
	int			i;
	cheatvar_t	*var;

	if ( !Q_strcmp(cl.configstrings[CS_MAXCLIENTS], "1")
		|| !cl.configstrings[CS_MAXCLIENTS][0] )
		return;		// single player can cheat

	// find all the cvars if we haven't done it yet
	if (!numcheatvars)
	{
		while (cheatvars[numcheatvars].name)
		{
			cheatvars[numcheatvars].var = Cvar_Get (cheatvars[numcheatvars].name,
					cheatvars[numcheatvars].value, 0);
			numcheatvars++;
		}
	}

	// make sure they are all set to the proper values
	for (i=0, var = cheatvars ; i<numcheatvars ; i++, var++)
	{
		if ( Q_strcmp (var->var->GetString(), var->value) )
		{
			Cvar_FindSetString (var->name, var->value);
		}
	}
}

//=================================================================================================

/*
========================
CL_SendCommand
========================
*/
static void CL_SendCommand()
{
	// allow mice or other external controllers to add commands
	input::Commands ();

	// process console commands
	Cbuf_Execute ();

	// fix any cheating cvars
	CL_FixCvarCheats ();

	// send intentions now
	CL_SendCmd ();

	// resend a connection request if necessary
	CL_CheckForResend ();
}

/*
========================
CL_Frame
========================
*/
void CL_Frame( int msec )
{
	static int extratime;
	static int lasttimecalled;

	if ( dedicated->GetBool() ) {
		return;
	}

	extratime += msec;

	if ( !cl_timedemo.GetBool() )
	{
		if ( cls.state == ca_connected && extratime < 100 ) {
			// don't flood packets out while connecting
			return;
		}
		if ( extratime < 1000.0f / cl_maxfps.GetFloat() ) {		// 1000.0f = seconds to milliseconds
			// framerate is too high
			return;
		}
	}

	// Let the mouse activate or deactivate
	input::Frame();

	// Decide the simulation time
	cls.frametime = MS2SEC( (float)extratime ); // SlartTime
	cl.time += extratime;
	cls.realtime = curtime;

	extratime = 0;

	// SlartTodo: don't think we want this...
	if ( cls.frametime > ( 1.0f / 5.0f ) ) {
		cls.frametime = ( 1.0f / 5.0f );
	}

	// If in the debugger last frame, don't timeout
	if ( msec > 5000 ) {
		cls.netchan.last_received = Sys_Milliseconds();
	}

	// Fetch results from server
	CL_ReadPackets();

	// Send a new command message to the server
	CL_SendCommand();

	// Predict all unacknowledged movements
	CL_PredictMovement();

	// Allow rendering DLL change
	if ( !cl.refresh_prepped && cls.state == ca_active ) {
		CL_PrepRefresh();
	}

	// Update the screen
	if ( com_speeds->GetBool() ) { time_before_ref = Sys_Milliseconds(); }
	SCR_UpdateScreen();
	if ( com_speeds->GetBool() ) { time_after_ref = Sys_Milliseconds(); }

	// Update audio
	S_Update( cl.refdef.vieworg, cl.v_forward, cl.v_right, cl.v_up );

	CDAudio_Update();

	// Advance local effects for next frame
	cge->Frame();

	SCR_RunCinematic();

	// Frame complete, up the counter
	++cls.framecount;

	// Log stats
	if ( com_logStats->GetBool() )
	{
		if ( cls.state == ca_active )
		{
			if ( !lasttimecalled )
			{
				lasttimecalled = Sys_Milliseconds();
				if ( log_stats_file )
					FileSystem::PrintFile( "0\n", log_stats_file );
			}
			else
			{
				int now = Sys_Milliseconds();

				if ( log_stats_file )
					FileSystem::PrintFileFmt( log_stats_file, "%d\n", now - lasttimecalled );
				lasttimecalled = now;
			}
		}
	}
}

/*
========================
CL_Init
========================
*/
void CL_Init()
{
	if ( dedicated->GetBool() ) {
		return;
	}

	VID_Init();
	S_Init();

	V_Init();

	net_message.data = net_message_buffer;
	net_message.maxsize = sizeof( net_message_buffer );

	M_Init();

	SCR_Init();
	cls.disable_screen = 1.0f;	// don't draw yet

	CDAudio_Init();
	CL_InitLocal();
	input::Init();

	CL_InitCGame();

	Cbuf_Execute();
}

/*
========================
CL_Shutdown

FIXME: this is a callback from Sys_Quit and Com_Error.  It would be better
to run quit through here before the final handoff to the sys code.
========================
*/
void CL_Shutdown()
{
	static bool isdown;

	if ( dedicated->GetBool() ) {
		return;
	}

	if ( isdown )
	{
		// recursive shutdown
		return;
	}
	isdown = true;

	CL_WriteConfiguration();

	CL_ShutdownCGame();

	M_Shutdown();
	CDAudio_Shutdown();
	S_Shutdown();
	input::Shutdown();
	SCR_Shutdown();
	VID_Shutdown();
}
