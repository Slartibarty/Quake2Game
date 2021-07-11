// Server code for moving users

#include "sv_local.h"

edict_t *sv_player;

/*
===================================================================================================

	User stringcmd execution

	sv_client and sv_player will be valid.

===================================================================================================
*/

/*
========================
SV_BeginDemoServer
========================
*/
static void SV_BeginDemoserver()
{
	char name[MAX_OSPATH];

	Q_sprintf_s( name, "demos/%s", sv.name );
	sv.demofile = FileSystem::OpenFileRead( name );
	if ( !sv.demofile )
	{
		Com_Errorf( "Couldn't open %s\n", name );
	}
}

/*
========================
SV_New_f

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
========================
*/
static void SV_New_f()
{
	int playernum;
	edict_t *ent;

	Com_DPrintf( "New() from %s\n", sv_client->name );

	if ( sv_client->state != cs_connected )
	{
		Com_Print( "New not valid -- already spawned\n" );
		return;
	}

	// demo servers just dump the file message
	if ( sv.state == ss_demo )
	{
		SV_BeginDemoserver();
		return;
	}

	//
	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//

	// send the serverdata
	MSG_WriteByte( &sv_client->netchan.message, svc_serverdata );
	MSG_WriteLong( &sv_client->netchan.message, PROTOCOL_VERSION );
	MSG_WriteLong( &sv_client->netchan.message, svs.spawncount );
	MSG_WriteByte( &sv_client->netchan.message, sv.attractloop );
	MSG_WriteString( &sv_client->netchan.message, "WackassNutty" );

	if ( sv.state == ss_cinematic || sv.state == ss_pic )
	{
		playernum = -1;
	}
	else
	{
		playernum = sv_client - svs.clients;
	}
	MSG_WriteShort( &sv_client->netchan.message, playernum );

	// send full levelname
	MSG_WriteString( &sv_client->netchan.message, sv.configstrings[CS_NAME] );

	//
	// game server
	// 
	if ( sv.state == ss_game )
	{
		// set up the entity for the client
		ent = EDICT_NUM( playernum + 1 );
		ent->s.number = playernum + 1;
		sv_client->edict = ent;
		memset( &sv_client->lastcmd, 0, sizeof( sv_client->lastcmd ) );

		// begin fetching configstrings
		MSG_WriteByte( &sv_client->netchan.message, svc_stufftext );
		MSG_WriteString( &sv_client->netchan.message, va( "cmd configstrings %i 0\n", svs.spawncount ) );
	}
}

/*
========================
SV_Configstrings_f
========================
*/
static void SV_Configstrings_f()
{
	int start;

	Com_DPrintf( "Configstrings() from %s\n", sv_client->name );

	if ( sv_client->state != cs_connected )
	{
		Com_Print( "configstrings not valid -- already spawned\n" );
		return;
	}

	// handle the case of a level changing while a client was connecting
	if ( atoi( Cmd_Argv( 1 ) ) != svs.spawncount )
	{
		Com_Print( "SV_Configstrings_f from different level\n" );
		SV_New_f();
		return;
	}

	start = atoi( Cmd_Argv( 2 ) );

	// write a packet full of data

	while ( sv_client->netchan.message.cursize < MAX_MSGLEN / 2
		&& start < MAX_CONFIGSTRINGS )
	{
		if ( sv.configstrings[start][0] )
		{
			MSG_WriteByte( &sv_client->netchan.message, svc_configstring );
			MSG_WriteShort( &sv_client->netchan.message, start );
			MSG_WriteString( &sv_client->netchan.message, sv.configstrings[start] );
		}
		start++;
	}

	// send next command

	if ( start == MAX_CONFIGSTRINGS )
	{
		MSG_WriteByte( &sv_client->netchan.message, svc_stufftext );
		MSG_WriteString( &sv_client->netchan.message, va( "cmd baselines %i 0\n", svs.spawncount ) );
	}
	else
	{
		MSG_WriteByte( &sv_client->netchan.message, svc_stufftext );
		MSG_WriteString( &sv_client->netchan.message, va( "cmd configstrings %i %i\n", svs.spawncount, start ) );
	}
}

/*
========================
SV_Baselines_f
========================
*/
static void SV_Baselines_f()
{
	int start;
	entity_state_t nullstate;
	entity_state_t *base;

	Com_DPrintf( "Baselines() from %s\n", sv_client->name );

	if ( sv_client->state != cs_connected )
	{
		Com_Print( "baselines not valid -- already spawned\n" );
		return;
	}

	// handle the case of a level changing while a client was connecting
	if ( atoi( Cmd_Argv( 1 ) ) != svs.spawncount )
	{
		Com_Print( "SV_Baselines_f from different level\n" );
		SV_New_f();
		return;
	}

	start = atoi( Cmd_Argv( 2 ) );

	memset( &nullstate, 0, sizeof( nullstate ) );

	// write a packet full of data

	while ( sv_client->netchan.message.cursize < MAX_MSGLEN / 2 && start < MAX_EDICTS )
	{
		base = &sv.baselines[start];
		if ( base->modelindex || base->sound || base->effects )
		{
			MSG_WriteByte( &sv_client->netchan.message, svc_spawnbaseline );
			MSG_WriteDeltaEntity( &nullstate, base, &sv_client->netchan.message, true, true );
		}
		start++;
	}

	// send next command

	if ( start == MAX_EDICTS )
	{
		MSG_WriteByte( &sv_client->netchan.message, svc_stufftext );
		MSG_WriteString( &sv_client->netchan.message, va( "precache %i\n", svs.spawncount ) );
	}
	else
	{
		MSG_WriteByte( &sv_client->netchan.message, svc_stufftext );
		MSG_WriteString( &sv_client->netchan.message, va( "cmd baselines %i %i\n", svs.spawncount, start ) );
	}
}

/*
========================
SV_Begin_f
========================
*/
static void SV_Begin_f()
{
	Com_DPrintf( "Begin() from %s\n", sv_client->name );

	// handle the case of a level changing while a client was connecting
	if ( atoi( Cmd_Argv( 1 ) ) != svs.spawncount )
	{
		Com_Print( "SV_Begin_f from different level\n" );
		SV_New_f();
		return;
	}

	sv_client->state = cs_spawned;

	// call the game begin function
	ge->ClientBegin( sv_player );

	Cbuf_InsertFromDefer();
}

//=================================================================================================

/*
========================
SV_NextDownload_f
========================
*/
static void SV_NextDownload_f()
{
	int r;
	int percent;
	int size;

	if ( !sv_client->download ) {
		return;
	}

	r = sv_client->downloadsize - sv_client->downloadcount;
	if ( r > 1024 ) {
		r = 1024;
	}

	MSG_WriteByte( &sv_client->netchan.message, svc_download );
	MSG_WriteShort( &sv_client->netchan.message, r );

	sv_client->downloadcount += r;
	size = sv_client->downloadsize;
	if ( !size ) {
		size = 1;
	}
	percent = sv_client->downloadcount * 100 / size;
	MSG_WriteByte( &sv_client->netchan.message, percent );
	SZ_Write( &sv_client->netchan.message,
		sv_client->download + sv_client->downloadcount - r, r );

	if ( sv_client->downloadcount != sv_client->downloadsize ) {
		return;
	}

	FileSystem::FreeFile( sv_client->download );
	sv_client->download = NULL;
}

/*
==================
SV_BeginDownload_f
==================
*/
static void SV_BeginDownload_f()
{
	extern cvar_t *allow_download;
	extern cvar_t *allow_download_players;
	extern cvar_t *allow_download_models;
	extern cvar_t *allow_download_sounds;
	extern cvar_t *allow_download_maps;

	char *name;
	int offset = 0;

	name = Cmd_Argv( 1 );

	if ( Cmd_Argc() > 2 ) {
		offset = atoi( Cmd_Argv( 2 ) ); // downloaded offset
	}

	// hacked by zoid to allow more conrol over download
	// first off, no .. or global allow check
	if ( strstr( name, ".." ) || !allow_download->GetBool()
		// leading dot is no good
		|| *name == '.'
		// leading slash bad as well, must be in subdir
		|| *name == '/'
		// next up, skin check
		|| ( Q_strncmp( name, "players/", 6 ) == 0 && !allow_download_players->GetBool() )
		// now models
		|| ( Q_strncmp( name, "models/", 6 ) == 0 && !allow_download_models->GetBool() )
		// now sounds
		|| ( Q_strncmp( name, "sound/", 6 ) == 0 && !allow_download_sounds->GetBool() )
		// now maps (note special case for maps, must not be in pak)
		|| ( Q_strncmp( name, "maps/", 6 ) == 0 && !allow_download_maps->GetBool() )
		// MUST be in a subdirectory	
		|| !strstr( name, "/" ) )
	{
		// don't allow anything with .. path
		MSG_WriteByte( &sv_client->netchan.message, svc_download );
		MSG_WriteShort( &sv_client->netchan.message, -1 );
		MSG_WriteByte( &sv_client->netchan.message, 0 );
		return;
	}

	if ( sv_client->download ) {
		FileSystem::FreeFile( sv_client->download );
	}

	sv_client->downloadsize = FileSystem::LoadFile( name, (void **)&sv_client->download );
	sv_client->downloadcount = offset;

	if ( offset > sv_client->downloadsize ) {
		sv_client->downloadcount = sv_client->downloadsize;
	}

	if ( !sv_client->download )
	{
		Com_DPrintf( "Couldn't download %s to %s\n", name, sv_client->name );
		if ( sv_client->download ) {
			FileSystem::FreeFile( sv_client->download );
			sv_client->download = NULL;
		}

		MSG_WriteByte( &sv_client->netchan.message, svc_download );
		MSG_WriteShort( &sv_client->netchan.message, -1 );
		MSG_WriteByte( &sv_client->netchan.message, 0 );
		return;
	}

	SV_NextDownload_f();
	Com_DPrintf( "Downloading %s to %s\n", name, sv_client->name );
}

//=================================================================================================

/*
========================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately
========================
*/
static void SV_Disconnect_f()
{
//	SV_EndRedirect();
	SV_DropClient( sv_client );
}

/*
========================
SV_ShowServerinfo_f

Dumps the serverinfo info string
========================
*/
static void SV_ShowServerinfo_f()
{
	Info_Print( Cvar_Serverinfo() );
}

void SV_Nextserver()
{
	//ZOID, ss_pic can be nextserver'd in coop mode
	if ( sv.state == ss_game || ( sv.state == ss_pic && !Cvar_FindGetFloat( "coop" ) ) ) {
		// can't nextserver while playing a normal game
		return;
	}

	svs.spawncount++;	// make sure another doesn't sneak in
	const char *v = Cvar_FindGetString( "nextserver" );
	if ( !v[0] )
	{
		Cbuf_AddText( "killserver\n" );
	}
	else
	{
		Cbuf_AddText( v );
		Cbuf_AddText( "\n" );
	}
	Cvar_FindSetString( "nextserver", "" );
}

/*
========================
SV_Nextserver_f

A cinematic has completed or been aborted by a client, so move
to the next server,
========================
*/
static void SV_Nextserver_f()
{
	if ( atoi( Cmd_Argv( 1 ) ) != svs.spawncount ) {
		Com_DPrintf( "Nextserver() from wrong level, from %s\n", sv_client->name );
		return;		// leftover from last server
	}

	Com_DPrintf( "Nextserver() from %s\n", sv_client->name );

	SV_Nextserver();
}

struct ucmd_t
{
	const char	*name;
	void		(*func) (void);
};

const ucmd_t ucmds[]
{
	// auto issued
	{"new", SV_New_f},
	{"configstrings", SV_Configstrings_f},
	{"baselines", SV_Baselines_f},
	{"begin", SV_Begin_f},

	{"nextserver", SV_Nextserver_f},

	{"disconnect", SV_Disconnect_f},

	// issued by hand at client consoles	
	{"info", SV_ShowServerinfo_f},

	{"download", SV_BeginDownload_f},
	{"nextdl", SV_NextDownload_f},

	{NULL, NULL}
};

/*
========================
SV_ExecuteUserCommand
========================
*/
void SV_ExecuteUserCommand( char *s )
{
	const ucmd_t *u;

	Cmd_TokenizeString( s, true );
	sv_player = sv_client->edict;

//	SV_BeginRedirect( RD_CLIENT );

	for ( u = ucmds; u->name; u++ )
	{
		if ( Q_strcmp( Cmd_Argv( 0 ), u->name ) == 0 )
		{
			u->func();
			break;
		}
	}

	if ( !u->name && sv.state == ss_game ) {
		ge->ClientCommand( sv_player );
	}

//	SV_EndRedirect();
}

/*
===================================================================================================

USER CMD EXECUTION

===================================================================================================
*/

static void SV_ClientThink( client_t *cl, usercmd_t *cmd )
{
	cl->commandMsec -= cmd->msec;

	if ( cl->commandMsec < 0 && sv_enforcetime->GetBool() )
	{
		Com_DPrintf( "commandMsec underflow from %s\n", cl->name );
		return;
	}

	ge->ClientThink( cl->edict, cmd );
}

/*
========================
SV_ExecuteClientMessage

The current net_message is parsed for the given client
========================
*/
#define	MAX_STRINGCMDS	8
void SV_ExecuteClientMessage( client_t *cl )
{
	int c;
	char *s;

	usercmd_t	nullcmd;
	usercmd_t	oldest, oldcmd, newcmd;
	int			net_drop;
	int			stringCmdCount;
	int			checksum, calculatedChecksum;
	int			checksumIndex;
	bool		move_issued;
	int			lastframe;

	sv_client = cl;
	sv_player = sv_client->edict;

	// only allow one move command
	move_issued = false;
	stringCmdCount = 0;

	while ( 1 )
	{
		if ( net_message.readcount > net_message.cursize )
		{
			Com_Print( "SV_ReadClientMessage: badread\n" );
			SV_DropClient( cl );
			return;
		}

		c = MSG_ReadByte( &net_message );
		if ( c == -1 ) {
			break;
		}

		switch ( c )
		{
		default:
			Com_Print( "SV_ReadClientMessage: unknown command char\n" );
			SV_DropClient( cl );
			return;

		case clc_nop:
			break;

		case clc_userinfo:
			Q_strcpy_s( cl->userinfo, MSG_ReadString( &net_message ) );
			SV_UserinfoChanged( cl );
			break;

		case clc_move:
			if ( move_issued ) {
				// someone is trying to cheat...
				return;
			}

			move_issued = true;
			checksumIndex = net_message.readcount;
			checksum = MSG_ReadByte( &net_message );
			lastframe = MSG_ReadLong( &net_message );
			if ( lastframe != cl->lastframe ) {
				cl->lastframe = lastframe;
				if ( cl->lastframe > 0 ) {
					cl->frame_latency[cl->lastframe & ( LATENCY_COUNTS - 1 )] =
						svs.realtime - cl->frames[cl->lastframe & UPDATE_MASK].senttime;
				}
			}

			memset( &nullcmd, 0, sizeof( nullcmd ) );
			MSG_ReadDeltaUsercmd( &net_message, &nullcmd, &oldest );
			MSG_ReadDeltaUsercmd( &net_message, &oldest, &oldcmd );
			MSG_ReadDeltaUsercmd( &net_message, &oldcmd, &newcmd );

			if ( cl->state != cs_spawned )
			{
				cl->lastframe = -1;
				break;
			}

			// if the checksum fails, ignore the rest of the packet
			calculatedChecksum = COM_BlockSequenceCRCByte(
				net_message.data + checksumIndex + 1,
				net_message.readcount - checksumIndex - 1,
				cl->netchan.incoming_sequence );

			if ( calculatedChecksum != checksum )
			{
				Com_DPrintf( "Failed command checksum for %s (%d != %d)/%d\n",
					cl->name, calculatedChecksum, checksum,
					cl->netchan.incoming_sequence );
				return;
			}

			if ( !sv_paused->GetBool() )
			{
				net_drop = cl->netchan.dropped;
				if ( net_drop < 20 )
				{

				//	if (net_drop > 2)

				//	Com_Printf ("drop %i\n", net_drop);
					while ( net_drop > 2 )
					{
						SV_ClientThink( cl, &cl->lastcmd );

						net_drop--;
					}
					if ( net_drop > 1 ) {
						SV_ClientThink( cl, &oldest );
					}

					if ( net_drop > 0 ) {
						SV_ClientThink( cl, &oldcmd );
					}

				}
				SV_ClientThink( cl, &newcmd );
			}

			cl->lastcmd = newcmd;
			break;

		case clc_stringcmd:
			s = MSG_ReadString( &net_message );

			// malicious users may try using too many string commands
			if ( ++stringCmdCount < MAX_STRINGCMDS ) {
				SV_ExecuteUserCommand( s );
			}

			if ( cl->state == cs_zombie ) {
				// disconnect command
				return;
			}
			break;
		}
	}
}
