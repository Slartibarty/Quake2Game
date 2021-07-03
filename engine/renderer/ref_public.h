
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
void		R_BeginFrame( bool imgui = false, int frameBuffer = 0 );	// should only be called by SCR_Update
void		R_RenderFrame( refdef_t *fd );
void		R_EndFrame( bool imgui = false );							// should only be called by SCR_Update

			// 2D elements, occurs after world and entities
void		R_DrawGetPicSize( int *w, int *h, const char *name );
void		R_DrawPic( int x, int y, const char *name );
void		R_DrawStretchPic( int x, int y, int w, int h, const char *name );
void		R_DrawCharColor( int x, int y, int ch, uint32 color );
void		R_DrawChar( int x, int y, int ch );
void		R_DrawFilled( int x, int y, int w, int h, uint32 color );
void		R_DrawStretchRaw( int x, int y, int w, int h, int cols, int rows, byte *data );
void		R_SetRawPalette( const byte *palette );

			// 3D elements
void		R_DrawBounds( const vec3_t mins, const vec3_t maxs );
void		R_DrawLine( const vec3_t start, const vec3_t end );

void *		R_GetWindowHandle();
void		R_AppActivate( bool active );

int			R_CreateFBO( int width, int height );
void		R_DestroyFBO( int fbo );
void		R_BindFBO( int fbo );
void		R_BindDefaultFBO();
uint		R_TexNumFBO( int fbo );
