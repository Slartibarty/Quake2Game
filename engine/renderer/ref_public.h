
#pragma once

#include "ref_types.h"

bool		R_Init();
void		R_Restart();
void		R_Shutdown();

			// Registration
void		R_BeginRegistration( const char *map );
model_t		*R_RegisterModel( const char *name );
material_t	*R_RegisterSkin( const char *name );
material_t	*R_RegisterPic( const char *name );
void		R_SetSky( const char *name, float rotate, vec3_t axis );
void		R_EndRegistration();

			// Render entry points
void		R_BeginFrame( bool imgui = false );
void		R_RenderFrame( refdef_t *fd );
void		R_EndFrame( bool imgui = false );

			// 2D elements, occurs after world and entities
void		R_DrawGetPicSize( int *w, int *h, const char *name );
void		R_DrawPic( int x, int y, const char *name );
void		R_DrawStretchPic( int x, int y, int w, int h, const char *name );
void		R_DrawCharColor( int x, int y, int ch, uint32 color );
void		R_DrawChar( int x, int y, int ch );
void		R_DrawFilled( int x, int y, int w, int h, uint32 color );
void		R_DrawStretchRaw( int x, int y, int w, int h, int cols, int rows, byte *data );
void		R_SetRawPalette( const byte *palette );

void *		R_GetWindowHandle();
void		R_AppActivate( bool active );
