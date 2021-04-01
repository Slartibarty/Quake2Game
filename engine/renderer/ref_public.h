//=================================================================================================
// Public refresh header
// 
// Contains types used both outside and inside of the renderer
//=================================================================================================

#pragma once

#include "ref_types.h"

bool		R_Init( void *hinstance, void *wndproc );
void		R_Shutdown();

			// Registration
void		R_BeginRegistration( const char *map );
model_t		*R_RegisterModel( const char *name );
material_t	*R_RegisterSkin( const char *name );
material_t	*R_RegisterPic( const char *name );
void		R_SetSky( const char *name, float rotate, vec3_t axis );
void		R_EndRegistration();

			// Render entry points
void		R_BeginFrame();
void		R_RenderFrame( refdef_t *fd );
void		R_EndFrame();

			// 2D elements, occurs after world and entities
void		Draw_GetPicSize( int *w, int *h, const char *name );
void		Draw_Pic( int x, int y, const char *name );
void		Draw_StretchPic( int x, int y, int w, int h, const char *name );
void		Draw_Char( int x, int y, int ch );
void		Draw_TileClear( int x, int y, int w, int h, const char *name );
void		R_DrawFilled( float x, float y, float w, float h, qColor color );

			// Draw raw image data, used by cinematics
void		Draw_StretchRaw( int x, int y, int w, int h, int cols, int rows, byte *data );

			// Set the palette used by cinematics
void		R_SetRawPalette( const unsigned char *palette );

			// Alert this system about the window state changing
void		GLimp_AppActivate( bool activate );
