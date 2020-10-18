//=================================================================================================
// Game main
//=================================================================================================

#include "ng_local.h"

game_import_t	gi;
game_export_t	globals;

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// This will be called when the dll is first loaded, which
// only happens when a new game is started or a save game is loaded.
//-------------------------------------------------------------------------------------------------
void InitGame()
{
	gi.dprintf("==== InitGame ====\n");
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ShutdownGame(void)
{
	gi.dprintf("==== ShutdownGame ====\n");
}

//-------------------------------------------------------------------------------------------------
// Advances the world by 0.1 seconds
//-------------------------------------------------------------------------------------------------
void G_RunFrame()
{
}

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Returns a pointer to the structure with all entry points
// and global variables
//-------------------------------------------------------------------------------------------------
game_export_t *GetGameAPI(game_import_t *import)
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

	globals.edict_size = sizeof(edict_t);

	return &globals;
}

//-------------------------------------------------------------------------------------------------

#ifndef GAME_HARD_LINKED

// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error(const char *error, ...)
{
	va_list		argptr;
	char		text[MAX_PRINT_MSG];

	va_start(argptr, error);
	Com_vsprintf(text, error, argptr);
	va_end(argptr);

	gi.error(ERR_FATAL, "%s", text);
}

void Com_Printf(const char *msg, ...)
{
	va_list		argptr;
	char		text[MAX_PRINT_MSG];

	va_start(argptr, msg);
	Com_vsprintf(text, msg, argptr);
	va_end(argptr);

	gi.dprintf("%s", text);
}

#endif
