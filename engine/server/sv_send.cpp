// Server main program

#include "sv_local.h"

/*
===================================================================================================

	Com_Printf redirection

===================================================================================================
*/

char sv_outputbuf[SV_OUTPUTBUF_LENGTH];

void SV_FlushRedirect( int sv_redirected, char *outputbuf )
{
	if ( sv_redirected == RD_PACKET )
	{
		Netchan_OutOfBandPrint( NS_SERVER, net_from, "print\n%s", outputbuf );
	}
	else if ( sv_redirected == RD_CLIENT )
	{
		MSG_WriteByte( &sv_client->netchan.message, svc_print );
		MSG_WriteByte( &sv_client->netchan.message, PRINT_HIGH );
		MSG_WriteString( &sv_client->netchan.message, outputbuf );
	}
}

/*
===================================================================================================

	Event messages

===================================================================================================
*/

/*
========================
SV_ClientPrintf

Sends text across to be displayed if the level passes
========================
*/
void SV_ClientPrintf( client_t *cl, int level, _Printf_format_string_ const char *fmt, ... )
{
	char	string[MAX_PRINT_MSG];
	va_list	argptr;

	if ( level < cl->messagelevel ) {
		return;
	}

	va_start( argptr, fmt );
	Q_vsprintf_s( string, fmt, argptr );
	va_end( argptr );

	MSG_WriteByte( &cl->netchan.message, svc_print );
	MSG_WriteByte( &cl->netchan.message, level );
	MSG_WriteString( &cl->netchan.message, string );
}

/*
========================
SV_BroadcastPrintf

Sends text to all active clients
========================
*/
void SV_BroadcastPrintf( int level, _Printf_format_string_ const char *fmt, ... )
{
	char		string[MAX_PRINT_MSG];
	va_list		argptr;
	client_t *	cl;
	int			i;

	va_start( argptr, fmt );
	Q_vsprintf_s( string, fmt, argptr );
	va_end( argptr );

	// echo to console
	if ( dedicated->GetBool() )
	{
		Com_Print( string );
	}

	for ( i = 0, cl = svs.clients; i < maxclients->GetInt32(); i++, cl++ )
	{
		if ( level < cl->messagelevel ) {
			continue;
		}
		if ( cl->state != cs_spawned ) {
			continue;
		}
		MSG_WriteByte( &cl->netchan.message, svc_print );
		MSG_WriteByte( &cl->netchan.message, level );
		MSG_WriteString( &cl->netchan.message, string );
	}
}

/*
========================
SV_BroadcastCommand

Sends text to all active clients
========================
*/
void SV_BroadcastCommand( const char *cmd )
{
	if ( !sv.state ) {
		return;
	}

	MSG_WriteByte( &sv.multicast, svc_stufftext );
	MSG_WriteString( &sv.multicast, cmd );
	SV_Multicast( NULL, MULTICAST_ALL_R );
}

/*
========================
SV_Multicast

Sends the contents of sv.multicast to a subset of the clients,
then clears sv.multicast.

MULTICAST_ALL	same as broadcast (origin can be NULL)
MULTICAST_PVS	send to clients potentially visible from org
MULTICAST_PHS	send to clients potentially hearable from org
========================
*/
void SV_Multicast( vec3_t origin, multicast_t to )
{
	client_t *	client;
	byte *		mask;
	int			leafnum, cluster;
	int			j;
	bool		reliable;
	int			area1, area2;

	reliable = false;

	if ( to != MULTICAST_ALL_R && to != MULTICAST_ALL )
	{
		leafnum = CM_PointLeafnum( origin );
		area1 = CM_LeafArea( leafnum );
	}
	else
	{
		leafnum = 0;	// just to avoid compiler warnings
		area1 = 0;
	}

	// if doing a serverrecord, store everything
	if ( svs.demofile ) {
		SZ_Write( &svs.demo_multicast, sv.multicast.data, sv.multicast.cursize );
	}

	switch ( to )
	{
	case MULTICAST_ALL_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_ALL:
		leafnum = 0;
		mask = nullptr;
		break;

	case MULTICAST_PHS_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_PHS:
		leafnum = CM_PointLeafnum( origin );
		cluster = CM_LeafCluster( leafnum );
		mask = CM_ClusterPHS( cluster );
		break;

	case MULTICAST_PVS_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_PVS:
		leafnum = CM_PointLeafnum( origin );
		cluster = CM_LeafCluster( leafnum );
		mask = CM_ClusterPVS( cluster );
		break;

	default:
		mask = nullptr;
		Com_FatalErrorf( "SV_Multicast: bad to:%i", to );
	}

	// send the data to all relevent clients
	for ( j = 0, client = svs.clients; j < maxclients->GetInt32(); j++, client++ )
	{
		if ( client->state == cs_free || client->state == cs_zombie ) {
			continue;
		}
		if ( client->state != cs_spawned && !reliable ) {
			continue;
		}

		if ( mask )
		{
			leafnum = CM_PointLeafnum( client->edict->s.origin );
			cluster = CM_LeafCluster( leafnum );
			area2 = CM_LeafArea( leafnum );
			if ( !CM_AreasConnected( area1, area2 ) ) {
				continue;
			}
			if ( mask && ( !( mask[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) ) ) {
				continue;
			}
		}

		if ( reliable ) {
			SZ_Write( &client->netchan.message, sv.multicast.data, sv.multicast.cursize );
		} else {
			SZ_Write( &client->datagram, sv.multicast.data, sv.multicast.cursize );
		}
	}

	SZ_Clear( &sv.multicast );
}

/*  
========================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

If channel & 8, the sound will be sent to everyone, not just
things in the PHS.

FIXME: if entity isn't in PHS, they must be forced to be sent or
have the origin explicitly sent.

Channel 0 is an auto-allocate channel, the others override anything
already running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)

Timeofs can range from 0.0 to 0.1 to cause sounds to be started
later in the frame than they normally would.

If origin is NULL, the origin is determined from the entity origin
or the midpoint of the entity box for bmodels.
========================
*/  
void SV_StartSound( vec3_t origin, edict_t *entity, int channel,
					int soundindex, float volume,
					float attenuation, float timeofs )
{
	int			sendchan;
	int			flags;
	int			i;
	int			ent;
	bool		use_phs;
	vec3_t		origin_v;

	if ( volume < 0.0f || volume > 1.0f ) {
		Com_FatalErrorf( "SV_StartSound: volume = %f", volume );
	}

	if ( attenuation < 0.0f || attenuation > 4.0f ) {
		Com_FatalErrorf( "SV_StartSound: attenuation = %f", attenuation );
	}

//	if (channel < 0 || channel > 15)
//		Com_FatalErrorf("SV_StartSound: channel = %i", channel);

	if ( timeofs < 0.0f || timeofs > 0.255f ) {
		Com_FatalErrorf( "SV_StartSound: timeofs = %f", timeofs );
	}

	ent = NUM_FOR_EDICT( entity );

	if ( channel & 8 )	// no PHS flag
	{
		use_phs = false;
		channel &= 7;
	}
	else
	{
		use_phs = true;
	}

	sendchan = ( ent << 3 ) | ( channel & 7 );

	flags = 0;
	if ( volume != DEFAULT_SOUND_PACKET_VOLUME ) {
		flags |= SND_VOLUME;
	}
	if ( attenuation != DEFAULT_SOUND_PACKET_ATTENUATION ) {
		flags |= SND_ATTENUATION;
	}

	// the client doesn't know that bmodels have weird origins
	// the origin can also be explicitly set
	if ( ( entity->svflags & SVF_NOCLIENT )
		|| ( entity->solid == SOLID_BSP )
		|| origin )
	{
		flags |= SND_POS;
	}

	// always send the entity number for channel overrides
	flags |= SND_ENT;

	if ( timeofs ) {
		flags |= SND_OFFSET;
	}

	// use the entity origin unless it is a bmodel or explicitly specified
	if ( !origin )
	{
		origin = origin_v;
		if ( entity->solid == SOLID_BSP )
		{
			for ( i = 0; i < 3; i++ ) {
				origin_v[i] = entity->s.origin[i] + 0.5f * ( entity->mins[i] + entity->maxs[i] );
			}
		}
		else
		{
			VectorCopy( entity->s.origin, origin_v );
		}
	}

	MSG_WriteByte( &sv.multicast, svc_sound );
	MSG_WriteByte( &sv.multicast, flags );
	MSG_WriteByte( &sv.multicast, soundindex );

	if ( flags & SND_VOLUME ) {
		MSG_WriteByte( &sv.multicast, volume * 255 );
	}
	if ( flags & SND_ATTENUATION ) {
		MSG_WriteByte( &sv.multicast, attenuation * 64 );
	}
	if ( flags & SND_OFFSET ) {
		MSG_WriteByte( &sv.multicast, timeofs * 1000 );
	}

	if ( flags & SND_ENT ) {
		MSG_WriteShort( &sv.multicast, sendchan );
	}

	if ( flags & SND_POS ) {
		MSG_WritePos( &sv.multicast, origin );
	}

	// if the sound doesn't attenuate,send it to everyone
	// (global radio chatter, voiceovers, etc)
	if ( attenuation == ATTN_NONE ) {
		use_phs = false;
	}

	if ( channel & CHAN_RELIABLE )
	{
		if ( use_phs ) {
			SV_Multicast( origin, MULTICAST_PHS_R );
		} else {
			SV_Multicast( origin, MULTICAST_ALL_R );
		}
	}
	else
	{
		if ( use_phs ) {
			SV_Multicast( origin, MULTICAST_PHS );
		} else {
			SV_Multicast( origin, MULTICAST_ALL );
		}
	}
}

/*
===================================================================================================

	Frame updates

===================================================================================================
*/

/*
========================
SV_SendClientDatagram
========================
*/
static void SV_SendClientDatagram( client_t *client )
{
	byte		msg_buf[MAX_MSGLEN];
	sizebuf_t	msg;

	SV_BuildClientFrame( client );

	SZ_Init( &msg, msg_buf, sizeof( msg_buf ) );
	msg.allowoverflow = true;

	// send over all the relevant entity_state_t
	// and the player_state_t
	SV_WriteFrameToClient( client, &msg );

	// copy the accumulated multicast datagram
	// for this client out to the message
	// it is necessary for this to be after the WriteEntities
	// so that entity references will be current
	if ( client->datagram.overflowed ) {
		Com_Printf( "WARNING: datagram overflowed for %s\n", client->name );
	}
	else {
		SZ_Write( &msg, client->datagram.data, client->datagram.cursize );
	}
	SZ_Clear( &client->datagram );

	if ( msg.overflowed )
	{
		// must have room left for the packet header
		Com_Printf( "WARNING: msg overflowed for %s\n", client->name );
		SZ_Clear( &msg );
	}

	// send the datagram
	Netchan_Transmit( &client->netchan, msg.cursize, msg.data );

	// record the size for rate estimation
	client->message_size[sv.framenum % RATE_MESSAGES] = msg.cursize;
}

/*
========================
SV_DemoCompleted
========================
*/
static void SV_DemoCompleted()
{
	if ( sv.demofile )
	{
		fclose( sv.demofile );
		sv.demofile = NULL;
	}
	SV_Nextserver();
}

/*
========================
SV_RateDrop

Returns true if the client is over its current
bandwidth estimation and should not be sent another packet
========================
*/
static bool SV_RateDrop( client_t *c )
{
	int		total;
	int		i;

	// never drop over the loopback
	if ( c->netchan.remote_address.type == NA_LOOPBACK ) {
		return false;
	}

	total = 0;

	for ( i = 0; i < RATE_MESSAGES; i++ )
	{
		total += c->message_size[i];
	}

	if ( total > c->rate )
	{
		c->surpressCount++;
		c->message_size[sv.framenum % RATE_MESSAGES] = 0;
		return true;
	}

	return false;
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages()
{
	int			i;
	client_t *	c;
	int			msglen;
	byte		msgbuf[MAX_MSGLEN];
	int			r;

	msglen = 0;

	// read the next demo message if needed
	if ( sv.state == ss_demo && sv.demofile )
	{
		if ( sv_paused->GetBool() )
		{
			msglen = 0;
		}
		else
		{
			// get the next message
			r = fread( &msglen, 4, 1, sv.demofile );
			if ( r != 1 )
			{
				SV_DemoCompleted();
				return;
			}
			msglen = LittleLong( msglen );
			if ( msglen == -1 )
			{
				SV_DemoCompleted();
				return;
			}
			if ( msglen > MAX_MSGLEN ) {
				Com_Error( "SV_SendClientMessages: msglen > MAX_MSGLEN" );
			}
			r = fread( msgbuf, msglen, 1, sv.demofile );
			if ( r != 1 )
			{
				SV_DemoCompleted();
				return;
			}
		}
	}

	// send a message to each connected client
	for ( i = 0, c = svs.clients; i < maxclients->GetInt32(); i++, c++ )
	{
		if ( !c->state ) {
			continue;
		}
		// if the reliable message overflowed,
		// drop the client
		if ( c->netchan.message.overflowed )
		{
			SZ_Clear( &c->netchan.message );
			SZ_Clear( &c->datagram );
			SV_BroadcastPrintf( PRINT_HIGH, "%s overflowed\n", c->name );
			SV_DropClient( c );
		}

		if ( sv.state == ss_cinematic || sv.state == ss_demo || sv.state == ss_pic )
		{
			Netchan_Transmit( &c->netchan, msglen, msgbuf );
		}
		else if ( c->state == cs_spawned )
		{
			// don't overrun bandwidth
			if ( SV_RateDrop( c ) ) {
				continue;
			}

			SV_SendClientDatagram( c );
		}
		else
		{
			// just update reliable	if needed
			if ( c->netchan.message.cursize || curtime - c->netchan.last_sent > 1000 ) {
				Netchan_Transmit( &c->netchan, 0, NULL );
			}
		}
	}
}
