// cl_cgame.c -- client system interaction with client game

#include "client.h"

cgame_export_t	*cge;

extern void CL_ShutdownCGame();

void CL_InitCGame()
{
	cgame_import_t cgi;

	if ( cge )
		CL_ShutdownCGame();

	cgi.printf = Com_Printf;
	cgi.dprintf = Com_DPrintf;
	cgi.errorf = Com_Error;

	cge = (cgame_export_t *)Sys_GetCGameAPI( &cgi );

	if ( !cge )
	{
		Com_Error( ERR_DROP, "failed to load cgame DLL" );
	}

	if ( cge->apiversion != CGAME_API_VERSION )
	{
		Com_Error( ERR_DROP, "cgame is version %d, not %d",
			cge->apiversion, CGAME_API_VERSION );
	}

	cge->Init();
}

void CL_ShutdownCGame()
{
	if ( !cge )
		return;
	cge->Shutdown();
	Sys_UnloadCGame();
	cge = nullptr;
}
