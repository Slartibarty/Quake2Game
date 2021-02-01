//=================================================================================================
// Public refresh header
// 
// Contains types used both outside and inside of the renderer
//=================================================================================================

#pragma once

#define	MAX_DLIGHTS			32
#define	MAX_ENTITIES		128
#define	MAX_PARTICLES		4096
#define	MAX_LIGHTSTYLES		256

#define POWERSUIT_SCALE		4.0f

struct model_t;
struct material_t;

struct entity_t
{
	model_t		*model;				// opaque type outside refresh
	vec3_t		angles;

	/*
	** most recent data
	*/
	vec3_t		origin;				// also used as RF_BEAM's "from"
	int			frame;				// also used as RF_BEAM's diameter

	/*
	** previous data for lerping
	*/
	vec3_t		oldorigin;			// also used as RF_BEAM's "to"
	int			oldframe;

	/*
	** misc
	*/
	float		backlerp;			// 0.0 = current, 1.0 = old
	int			skinnum;			// also used as RF_BEAM's palette index

	int			lightstyle;			// for flashing entities
	float		alpha;				// ignore if RF_TRANSLUCENT isn't set

	material_t	*skin;				// NULL for inline skin
	int			flags;

};

struct dlight_t
{
	vec3_t	origin;
	vec3_t	color;
	float	intensity;
};

struct particle_t
{
	vec3_t	origin;
	int		color;
	float	alpha;
};

struct lightstyle_t
{
	vec3_t		rgb;			// 0.0 - 2.0
	float		white;			// highest of rgb
};

struct refdef_t
{
	int			x, y, width, height;// in virtual screen coordinates
	float		fov_x, fov_y;
	vec3_t		vieworg;
	vec3_t		viewangles;
	vec4_t		blend;				// rgba 0-1 full screen blend
	float		time;				// time is uesed to auto animate
	float		frametime;			// time since last frame, in milliseconds
	int			rdflags;			// RDF_UNDERWATER, etc

	byte		*areabits;			// if not NULL, only areas with set bits will be drawn

	lightstyle_t	*lightstyles;	// [MAX_LIGHTSTYLES]

	int			num_entities;
	entity_t	*entities;

	int			num_dlights;
	dlight_t	*dlights;

	int			num_particles;
	particle_t	*particles;
};

//-------------------------------------------------------------------------------------------------

bool		R_Init( void *hinstance, void *wndproc );
void		R_Shutdown();

//			Registration
void		R_BeginRegistration( const char *map );
model_t		*R_RegisterModel( const char *name );
material_t	*R_RegisterSkin( const char *name );
material_t	*Draw_FindPic( const char *name );
void		R_SetSky( const char *name, float rotate, vec3_t axis );
void		R_EndRegistration();

//			Render entry points
void		R_BeginFrame();
void		R_RenderFrame( refdef_t *fd );
void		R_EndFrame();

//			2D elements, occurs after world and entities
void		Draw_GetPicSize( int *w, int *h, const char *name );
void		Draw_Pic( int x, int y, const char *name );
void		Draw_StretchPic( int x, int y, int w, int h, const char *name );
void		Draw_Char( int x, int y, int c );
void		Draw_TileClear( int x, int y, int w, int h, const char *name );
void		Draw_Fill( int x, int y, int w, int h, int c );
void		Draw_FadeScreen();

//			Draw raw image data, used by cinematics
void		Draw_StretchRaw( int x, int y, int w, int h, int cols, int rows, byte *data );

//			Set the palette used by cinematics
void		R_SetRawPalette( const unsigned char *palette );

//			Alert this system about the window state changing
void		GLimp_AppActivate( bool activate );
