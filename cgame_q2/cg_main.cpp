
#include "cg_local.h"

cgame_import_t	cgi;
cgame_export_t	cge;

/*
===================
CG_Init

Called just after the DLL is loaded
===================
*/
void CG_Init()
{
	cgi.Printf( "==== CG_Init ====\n" );
}

/*
===================
CG_Shutdown

Called before the DLL is unloaded
===================
*/
void CG_Shutdown()
{
	cgi.Printf( "==== CG_Shutdown ====\n" );
}


/*
===================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
===================
*/
cgame_export_t *GetCGameAPI( cgame_import_t *import )
{
	cgi = *import;

	cge.apiversion = CGAME_API_VERSION;
	cge.Init = CG_Init;
	cge.Shutdown = CG_Shutdown;

	return &cge;
}

#ifndef GAME_HARD_LINKED

// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		text[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( text, fmt, argptr );
	va_end( argptr );

	cgi.Errorf( ERR_FATAL, "%s", text );
}

void Com_Printf( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		text[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( text, fmt, argptr );
	va_end( argptr );

	cgi.Printf( "%s", text );
}

#endif
