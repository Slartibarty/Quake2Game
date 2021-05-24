
#include "sv_local.h"

serverStatic_t	svs;		// persistant server info
server_t		sv;			// local server

/*
========================
SV_FindIndex
========================
*/
static int SV_FindIndex( const char *name, int start, int max, bool create )
{
	int		i;

	if ( !name || !name[0] ) {
		return 0;
	}

	for ( i = 1; i < max && sv.configstrings[start + i][0]; i++ )
	{
		if ( Q_strcmp( sv.configstrings[start + i], name ) == 0 ) {
			return i;
		}
	}

	if ( !create ) {
		return 0;
	}

	if ( i == max ) {
		Com_Error( "*Index: overflow" );
	}

	Q_strcpy_s( sv.configstrings[start + i], name );

	if ( sv.state != ss_loading )
	{
		// send the update to everyone
		SZ_Clear( &sv.multicast );
		MSG_WriteChar( &sv.multicast, svc_configstring );
		MSG_WriteShort( &sv.multicast, start + i );
		MSG_WriteString( &sv.multicast, name );
		SV_Multicast( vec3_origin, MULTICAST_ALL_R );
	}

	return i;
}

int SV_ModelIndex( const char *name )
{
	return SV_FindIndex( name, CS_MODELS, MAX_MODELS, true );
}

int SV_SoundIndex( const char *name )
{
	return SV_FindIndex( name, CS_SOUNDS, MAX_SOUNDS, true );
}

int SV_ImageIndex( const char *name )
{
	return SV_FindIndex( name, CS_IMAGES, MAX_IMAGES, true );
}

/*
========================
SV_CreateBaseline

Entity baselines are used to compress the update messages
to the clients -- only the fields that differ from the
baseline will be transmitted
========================
*/
static void SV_CreateBaseline()
{
	edict_t *		svent;
	int				entnum;

	for ( entnum = 1; entnum < ge->num_edicts; entnum++ )
	{
		svent = EDICT_NUM( entnum );
		if ( !svent->inuse ) {
			continue;
		}
		if ( !svent->s.modelindex && !svent->s.sound && !svent->s.effects ) {
			continue;
		}
		svent->s.number = entnum;

		//
		// take current state as baseline
		//
		VectorCopy( svent->s.origin, svent->s.old_origin );
		sv.baselines[entnum] = svent->s;
	}
}

/*
========================
SV_CheckForSavegame
========================
*/
static void SV_CheckForSavegame()
{
	char		name[MAX_OSPATH];
	FILE *		f;

	if ( sv_noreload->GetBool() ) {
		return;
	}

	if ( Cvar_VariableValue( "deathmatch" ) ) {
		return;
	}

	Q_sprintf_s( name, "%s/save/current/%s.sav", FS_Gamedir(), sv.name );
	f = fopen( name, "rb" );
	if ( !f ) {
		// no savegame
		return;
	}

	fclose( f );

	SV_ClearWorld();

	// get configstrings and areaportals
	SV_ReadLevelFile();

#if 0
	if ( !sv.loadgame )
	{	// coming back to a level after being in a different
		// level, so run it for ten seconds

		// rlava2 was sending too many lightstyles, and overflowing the
		// reliable data. temporarily changing the server state to loading
		// prevents these from being passed down.
		serverState_t		previousState;		// PGM

		previousState = sv.state;				// PGM
		sv.state = ss_loading;					// PGM
		for ( i = 0; i < 100; i++ )
			ge->RunFrame();

		sv.state = previousState;				// PGM
	}
#endif
}

/*
========================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.
========================
*/
static void SV_SpawnServer( const char *server, const char *spawnpoint, serverState_t serverstate, bool attractloop, bool loadgame )
{
	int			i;
	uint		checksum;

	if ( attractloop ) {
		Cvar_Set( "paused", "0" );
	}

	Com_Print( "------- Server Initialization -------\n" );

	Com_DPrintf( "SpawnServer: %s\n", server );
	if ( sv.demofile ) {
		fclose( sv.demofile );
	}

	svs.spawncount++;		// any partially connected client will be restarted

	sv.state = ss_dead;
	Com_SetServerState( sv.state );

	// wipe the entire per-level structure
	memset( &sv, 0, sizeof( sv ) );
	svs.realtime = 0;
	sv.loadgame = loadgame;
	sv.attractloop = attractloop;

	// save name for levels that don't set message
	Q_strcpy_s( sv.configstrings[CS_NAME], server );

	SZ_Init( &sv.multicast, sv.multicast_buf, sizeof( sv.multicast_buf ) );

	Q_strcpy_s( sv.name, server );

	// leave slots at start for clients only
	for ( i = 0; i < maxclients->value; i++ )
	{
		// needs to reconnect
		if ( svs.clients[i].state > cs_connected ) {
			svs.clients[i].state = cs_connected;
		}
		svs.clients[i].lastframe = -1;
	}

	sv.time = 1000;

	Q_strcpy_s( sv.name, server );
	Q_strcpy_s( sv.configstrings[CS_NAME], server );

	if ( serverstate != ss_game )
	{
		sv.models[1] = nullptr;		// no real map
		checksum = 0;
	}
	else
	{
		Q_sprintf_s( sv.configstrings[CS_MODELS + 1], "maps/%s.bsp", server );
		sv.models[1] = CM_LoadMap( sv.configstrings[CS_MODELS + 1], false, &checksum );
	}
	Q_sprintf_s( sv.configstrings[CS_MAPCHECKSUM], "%u", checksum );

	//
	// clear physics interaction links
	//
	SV_ClearWorld();

	for ( i = 1; i < CM_NumInlineModels(); i++ )
	{
		Q_sprintf_s( sv.configstrings[CS_MODELS + 1 + i], "*%i", i );
		sv.models[i + 1] = CM_InlineModel( sv.configstrings[CS_MODELS + 1 + i] );
	}

	//
	// spawn the rest of the entities on the map
	//

	// precache and static commands can be issued during
	// map initialization
	sv.state = ss_loading;
	Com_SetServerState( sv.state );

	// load and spawn all other entities
	ge->SpawnEntities( sv.name, CM_EntityString(), spawnpoint );

	// run two frames to allow everything to settle
	//ge->RunFrame();
	//ge->RunFrame();

	// all precaches are complete
	sv.state = serverstate;
	Com_SetServerState( sv.state );

	// create a baseline for more efficient communications
	SV_CreateBaseline();

	// check for a savegame
	SV_CheckForSavegame();

	// set serverinfo variable
	Cvar_FullSet( "mapname", sv.name, CVAR_SERVERINFO | CVAR_NOSET );

	Com_Print( "-------------------------------------\n" );
}

/*
========================
SV_InitGame

A brand new game has been started
========================
*/
static constexpr auto idMasterAddress = "192.246.40.37:" STRINGIFY( PORT_MASTER );
void SV_InitGame()
{
	int		i;
	edict_t *ent;

	if ( svs.initialized ) {
		// cause any connected clients to reconnect
		SV_Shutdown( "Server restarted\n", true );
	} else {
		// make sure the client is down
		CL_Drop();
		SCR_BeginLoadingPlaque();
	}

	// get any latched variable changes (maxclients, etc)
	Cvar_GetLatchedVars();

	svs.initialized = true;

	if ( Cvar_VariableValue( "coop" ) && Cvar_VariableValue( "deathmatch" ) )
	{
		Com_Print( "Deathmatch and Coop both set, disabling Coop\n" );
		Cvar_FullSet( "coop", "0", CVAR_SERVERINFO | CVAR_LATCH );
	}

	// dedicated servers are can't be single player and are usually DM
	// so unless they explicity set coop, force it to deathmatch
	if ( dedicated->GetBool() )
	{
		if ( !Cvar_VariableValue( "coop" ) ) {
			Cvar_FullSet( "deathmatch", "1", CVAR_SERVERINFO | CVAR_LATCH );
		}
	}

	// init clients
	if ( Cvar_VariableValue( "deathmatch" ) )
	{
		if ( maxclients->GetInt32() <= 1 ) {
			Cvar_FullSet( "maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH );
		}
		else if ( maxclients->GetInt32() > MAX_CLIENTS ) {
			Cvar_FullSet( "maxclients", va( "%i", MAX_CLIENTS ), CVAR_SERVERINFO | CVAR_LATCH );
		}
	}
	else if ( Cvar_VariableValue( "coop" ) )
	{
		if ( maxclients->GetInt32() <= 1 || maxclients->GetInt32() > 4 ) {
			Cvar_FullSet( "maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH );
		}
	}
	else	// non-deathmatch, non-coop is one player
	{
		Cvar_FullSet( "maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH );
	}

	svs.spawncount = rand();
	svs.clients = (client_t *)Mem_ClearedAlloc( sizeof( client_t ) * maxclients->GetInt32() );
	svs.num_client_entities = maxclients->GetInt32() * UPDATE_BACKUP * 64;
	svs.client_entities = (entity_state_t *)Mem_ClearedAlloc( sizeof( entity_state_t ) * svs.num_client_entities );

	// init network stuff
	NET_Config( ( maxclients->GetInt32() > 1 ) );

	// heartbeats will always be sent to the id master
	svs.last_heartbeat = -99999;		// send immediately
	NET_StringToNetadr( idMasterAddress, master_adr[0] );

	// init game
	SV_InitGameProgs();

	for ( i = 0; i < maxclients->GetInt32(); i++ )
	{
		ent = EDICT_NUM( i + 1 );
		ent->s.number = i + 1;
		svs.clients[i].edict = ent;
		memset( &svs.clients[i].lastcmd, 0, sizeof( svs.clients[i].lastcmd ) );
	}
}

/*
========================
SV_Map

  the full syntax is:

  map [*]<map>$<startspot>+<nextserver>

command from the console or progs.
Map can also be a.cin, .pcx, or .dm2 file
Nextserver is used to allow a cinematic to play, then proceed to
another level:

	map tram.cin+jail_e3
========================
*/
void SV_Map( bool attractloop, const char *levelstring, bool loadgame )
{
	char	level[MAX_QPATH];
	char	spawnpoint[MAX_QPATH];

	sv.loadgame = loadgame;
	sv.attractloop = attractloop;

	if ( sv.state == ss_dead && !sv.loadgame ) {
		// the game is just starting
		SV_InitGame();
	}

	Q_strcpy_s( level, levelstring );

	// if there is a + in the map, set nextserver to the remainder
	char *ch = strstr( level, "+" );
	if ( ch ) {
		*ch = 0;
		Cvar_Set( "nextserver", va( "gamemap \"%s\"", ch + 1 ) );
	} else {
		Cvar_Set( "nextserver", "" );
	}

	//ZOID special hack for end game screen in coop mode
	if ( Cvar_VariableValue( "coop" ) && !Q_stricmp( level, "victory.pcx" ) ) {
		Cvar_Set( "nextserver", "gamemap \"*base1\"" );
	}

	// if there is a $, use the remainder as a spawnpoint
	ch = strstr( level, "$" );
	if ( ch ) {
		*ch = 0;
		Q_strcpy_s( spawnpoint, ch + 1 );
	} else {
		spawnpoint[0] = 0;
	}

	// skip the end-of-unit flag if necessary
	if ( level[0] == '*' ) {
		// Slart: This was pointed out by Address Sanitizer
		// level overlaps, which should be fine? Right?
		// Doing this for now
		memmove( level, level + 1, strlen( level ) );
	//	strcpy( level, level + 1 );
	}

	strlen_t l = Q_strlen( level );
	if ( l > 4 && Q_strcmp( level + l - 4, ".cin" ) == 0 )
	{
		SCR_BeginLoadingPlaque();			// for local system
		SV_BroadcastCommand( "changing\n" );
		SV_SpawnServer( level, spawnpoint, ss_cinematic, attractloop, loadgame );
	}
	else if ( l > 4 && Q_strcmp( level + l - 4, ".dm2" ) == 0 )
	{
		SCR_BeginLoadingPlaque();			// for local system
		SV_BroadcastCommand( "changing\n" );
		SV_SpawnServer( level, spawnpoint, ss_demo, attractloop, loadgame );
	}
	else if ( l > 4 && Q_strcmp( level + l - 4, ".pcx" ) == 0 )
	{
		SCR_BeginLoadingPlaque();			// for local system
		SV_BroadcastCommand( "changing\n" );
		SV_SpawnServer( level, spawnpoint, ss_pic, attractloop, loadgame );
	}
	else
	{
		SCR_BeginLoadingPlaque();			// for local system
		SV_BroadcastCommand( "changing\n" );
		SV_SendClientMessages();
		SV_SpawnServer( level, spawnpoint, ss_game, attractloop, loadgame );
		Cbuf_CopyToDefer();
	}

	SV_BroadcastCommand( "reconnect\n" );
}
