
#include "g_local.h"

game_locals_t	game;
level_locals_t	level;
game_import_t	gi;
game_export_t	globals;
spawn_temp_t	st;

edict_t		*g_edicts;

cvar_t	*deathmatch;
cvar_t	*coop;
cvar_t	*dmflags;
cvar_t	*skill;
cvar_t	*fraglimit;
cvar_t	*timelimit;
cvar_t	*maxclients;
cvar_t	*maxspectators;
cvar_t	*maxentities;
cvar_t	*dedicated;

cvar_t	*sv_maxvelocity;
cvar_t	*sv_gravity;

cvar_t	*g_viewthing;
cvar_t	*g_frametime;
cvar_t	*g_playersOnly;

cvar_t	*sv_cheats;

void SpawnEntities (const char *mapname, char *entities, const char *spawnpoint);
void ClientThink (edict_t *ent, usercmd_t *cmd);
bool ClientConnect (edict_t *ent, char *userinfo);
void ClientUserinfoChanged (edict_t *ent, char *userinfo);
void ClientDisconnect (edict_t *ent);
void ClientBegin (edict_t *ent);


//=================================================================================================

/*
========================
InitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
========================
*/
void InitGame()
{
	gi.dprintf( "==== InitGame ====\n" );

	//FIXME: sv_ prefix is wrong for these
	sv_maxvelocity = gi.cvar( "sv_maxvelocity", "2000", 0 );
	sv_gravity = gi.cvar( "sv_gravity", "800", 0 );

	// noset vars
	dedicated = gi.cvar( "dedicated", "0", CVAR_INIT );

	// latched vars
	sv_cheats = gi.cvar( "sv_cheats", "0", CVAR_SERVERINFO | CVAR_LATCH );

	maxclients = gi.cvar( "maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH );
	maxspectators = gi.cvar( "maxspectators", "4", CVAR_SERVERINFO );
	deathmatch = gi.cvar( "deathmatch", "0", CVAR_LATCH );
	coop = gi.cvar( "coop", "0", CVAR_LATCH );
	skill = gi.cvar( "skill", "1", CVAR_LATCH );
	maxentities = gi.cvar( "maxentities", "1024", CVAR_LATCH );

	// change anytime vars
	dmflags = gi.cvar( "dmflags", "0", CVAR_SERVERINFO );
	fraglimit = gi.cvar( "fraglimit", "0", CVAR_SERVERINFO );
	timelimit = gi.cvar( "timelimit", "0", CVAR_SERVERINFO );

	g_viewthing = gi.cvar( "g_viewthing", "models/devtest/barneyhl1.smf", CVAR_ARCHIVE );
	g_frametime = gi.cvar( "g_frametime", "0.1", 0 );
	g_playersOnly = gi.cvar( "g_playersOnly", "0", 0 );

	game.helpmessage1[0] = '\0';
	game.helpmessage2[0] = '\0';

	// initialize all entities for this game
	game.maxclients = maxclients->GetInt();
	game.maxentities = maxentities->GetInt();

	g_edicts = (edict_t*)gi.TagMalloc( game.maxentities * sizeof( g_edicts[0] ), TAG_GAME );

	game.clients = (gclient_t*)gi.TagMalloc( game.maxclients * sizeof( game.clients[0] ), TAG_GAME );
}

/*
========================
ShutdownGame
========================
*/
void ShutdownGame()
{
	gi.dprintf ("==== ShutdownGame ====\n");

	gi.FreeTags (TAG_LEVEL);
	gi.FreeTags (TAG_GAME);
}

//======================================================================

/*
========================
ClientEndServerFrames
========================
*/
static void ClientEndServerFrames()
{
	// Calc the player views now that all pushing
	// and damage has been added
	for ( int i = 1; i < game.maxclients; ++i )
	{
		CBaseEntity *pEntity = g_entityList[i];

		ClientEndServerFrame( pEntity );
	}
}

/*
========================
G_RunFrame

Advances the world
========================
*/
void G_RunFrame()
{
	level.framenum++;
	level.time = level.framenum*FRAMETIME;

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	for ( int i = 0; i < Ent_GetNumEntities(); ++i )
	{
		CBaseEntity *pEntity = Ent_GetEntity( i );

		VectorCopy( pEntity->m_edict.s.origin, pEntity->m_edict.s.old_origin );

		if ( i > 0 && i <= game.maxclients )
		{
			ClientBeginServerFrame( pEntity );
			continue;
		}

		Ent_RunThink( pEntity );
	}

	// build the playerstate_t structures for all players
	ClientEndServerFrames();
}

edict_t *GetEdict( int i )
{
	return &g_entityList[i]->m_edict;
}

int GetNumEdicts()
{
	return Ent_GetNumEntities();
}

//=================================================================================================

/*
========================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
========================
*/
extern "C" DLLEXPORT game_export_t * GetGameAPI( game_import_t * import )
{
	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;

	globals.WriteGame = WriteGame;
	globals.ReadGame = ReadGame;
	globals.WriteLevel = WriteLevel;
	globals.ReadLevel = ReadLevel;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;

	globals.RunFrame = G_RunFrame;

	globals.ServerCommand = ServerCommand;

	globals.GetEdict = GetEdict;
	globals.GetNumEdicts = GetNumEdicts;

	return &globals;
}

/*
===========================================================
	Core logging system implementation
===========================================================
*/

#ifndef GAME_HARD_LINKED

void Com_Print( const char *msg )
{
	gi.dprintf( "%s", msg );
}

void Com_Printf( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Print( msg );
}

void Com_DPrint( const char *msg )
{
	gi.dprintf( "%s", msg );
}

void Com_DPrintf( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_DPrint( msg );
}

[[noreturn]]
void Com_Error( const char *msg )
{
	gi.error( "%s", msg );
}

[[noreturn]]
void Com_Errorf( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Error( msg );
}

[[noreturn]]
void Com_FatalError( const char *msg )
{
	gi.error( "%s", msg );
}

[[noreturn]]
void Com_FatalErrorf( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_FatalError( msg );
}

#endif
