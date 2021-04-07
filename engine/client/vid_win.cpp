
#include "client.h"

/*
===================================================================================================

	A partially obsolete wrapper for the renderer

===================================================================================================
*/

viddef_t	viddef;

static bool	s_rendererActive;

/*
===================================================================================================

	Renderer stuff

===================================================================================================
*/

/*
========================
VID_Restart
========================
*/
static void VID_Restart_f()
{
	cls.disable_screen = true;

	R_Restart();

	cls.disable_screen = false;

	cl.refresh_prepped = false;
}

/*
========================
VID_NewWindow

Called by the renderer, sets the structure size
========================
*/
void VID_NewWindow ( int width, int height)
{
	viddef.width  = width;
	viddef.height = height;

	cl.force_refdef = true;		// can't use a paused refdef
}

/*
========================
VID_LoadRefresh
========================
*/
static bool VID_LoadRefresh()
{
	if ( s_rendererActive ) {
		Com_FatalError( "Tried to initialise the renderer twice!\nTell Slarti if this should be an error or not!" );
	}

	if ( R_Init() == false )
	{
		R_Shutdown();
		return false;
	}

	s_rendererActive = true;

	return true;
}

/*
========================
VID_Init
========================
*/
void VID_Init()
{
	Cmd_AddCommand( "vid_restart", VID_Restart_f );
	
	cls.disable_screen = true;

	if ( !VID_LoadRefresh() )
	{
		Com_FatalError( "Couldn't load renderer!" );
	}

	cls.disable_screen = false;

	cl.refresh_prepped = false;
}

/*
========================
VID_Shutdown
========================
*/
void VID_Shutdown()
{
	if ( s_rendererActive )
	{
		R_Shutdown();
		s_rendererActive = false;
	}
}
