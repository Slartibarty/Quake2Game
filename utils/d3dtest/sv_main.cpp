
#include "sv_local.h"

#include "game_server/svg_public.h"

void SV_Init()
{
	SVG_Init();
}

void SV_Shutdown()
{
	SVG_Shutdown();
}

void SV_Frame( float deltaTime )
{
	SVG_Frame( deltaTime );
}
