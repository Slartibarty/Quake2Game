
#pragma once

#define	MAX_DLIGHTS		32
#define	MAX_ENTITIES	128
#define	MAX_PARTICLES	4096
#define	MAX_LIGHTSTYLES	256

#define POWERSUIT_SCALE		4.0F

#define SHELL_RED_COLOR		0xF2
#define SHELL_GREEN_COLOR	0xD0
#define SHELL_BLUE_COLOR	0xF3

#define SHELL_RG_COLOR		0xDC
//#define SHELL_RB_COLOR		0x86
#define SHELL_RB_COLOR		0x68
#define SHELL_BG_COLOR		0x78

//ROGUE
#define SHELL_DOUBLE_COLOR	0xDF // 223
#define	SHELL_HALF_DAM_COLOR	0x90
#define SHELL_CYAN_COLOR	0x72
//ROGUE

#define SHELL_WHITE_COLOR	0xD7

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


#ifndef REF_HARD_LINKED

#define	API_VERSION		5

//
// these are the functions exported by the refresh module
//
struct refexport_t
{
	// if api_version is different, the dll cannot be used
	int		api_version;

	// called when the library is loaded
	bool	(*Init) ( void *hinstance, void *wndproc );

	// called before the library is unloaded
	void	(*Shutdown) (void);

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// EndRegistration will free any remaining data that wasn't registered.
	// Any model_s or skin_s pointers from before the BeginRegistration
	// are no longer valid after EndRegistration.
	//
	// Skins and images need to be differentiated, because skins
	// are flood filled to eliminate mip map edge errors, and pics have
	// an implicit "pics/" prepended to the name. (a pic name that starts with a
	// slash will not use the "pics/" prefix or the ".pcx" postfix)
	void	(*BeginRegistration) (const char *map);
	model_t *(*RegisterModel) (const char *name);
	image_t *(*RegisterSkin) (const char *name);
	image_t *(*RegisterPic) (const char *name);
	void	(*SetSky) (const char *name, float rotate, vec3_t axis);
	void	(*EndRegistration) (void);

	void	(*RenderFrame) (refdef_t *fd);

	void	(*DrawGetPicSize) (int *w, int *h, const char *name);	// will return 0 0 if not found
	void	(*DrawPic) (int x, int y, const char *name);
	void	(*DrawStretchPic) (int x, int y, int w, int h, const char *name);
	void	(*DrawChar) (int x, int y, int c);
	void	(*DrawTileClear) (int x, int y, int w, int h, const char *name);
	void	(*DrawFill) (int x, int y, int w, int h, int c);
	void	(*DrawFadeScreen) (void);

	// Draw images for cinematic rendering (which can have a different palette). Note that calls
	void	(*DrawStretchRaw) (int x, int y, int w, int h, int cols, int rows, byte *data);

	/*
	** video mode and refresh state management entry points
	*/
	void	(*CinematicSetPalette)( const unsigned char *palette);	// NULL = game palette
	void	(*BeginFrame)(void);
	void	(*EndFrame) (void);

	void	(*AppActivate)( bool activate );
};

//
// these are the functions imported by the refresh module
//
struct refimport_t
{
	void	(*Sys_Error) (int err_level, const char *str, ...);

	void	(*Cmd_AddCommand) (const char *name, void(*cmd)(void));
	void	(*Cmd_RemoveCommand) (const char *name);
	int		(*Cmd_Argc) (void);
	char	*(*Cmd_Argv) (int i);

	void	(*Con_Printf) (int print_level, const char *str, ...);

	// files will be memory mapped read only
	// the returned buffer may be part of a larger pak file,
	// or a discrete file from anywhere in the quake search path
	// a -1 return means the file does not exist
	// NULL can be passed for buf to just determine existance
	int		(*FS_LoadFile) (const char *name, void **buf);
	void	(*FS_FreeFile) (void *buf);

	// gamedir will be the current directory that generated
	// files should be stored to, ie: "f:\quake\id1"
	const char	*(*FS_Gamedir) (void);

	cvar_t	*(*Cvar_Get) (const char *name, const char *value, int flags);
	cvar_t	*(*Cvar_Set)( const char *name, const char *value );
	void	 (*Cvar_SetValue)( const char *name, float value );

	bool		(*Vid_GetModeInfo)( int *width, int *height, int mode );
	void		(*Vid_NewWindow)( int width, int height );
};

// this is the only function actually exported at the linker level
typedef	refexport_t	(*GetRefAPI_t) (refimport_t);

#else

bool	R_Init ( void *hinstance, void *wndproc );

void	R_Shutdown (void);

void	R_BeginRegistration (const char *map);
model_t *R_RegisterModel (const char *name);
material_t *R_RegisterSkin (const char *name);
material_t *Draw_FindPic (const char *name);
void	R_SetSky (const char *name, float rotate, vec3_t axis);
void	R_EndRegistration (void);

void	R_RenderFrame (refdef_t *fd);

void	Draw_GetPicSize (int *w, int *h, const char *name);
void	Draw_Pic (int x, int y, const char *name);
void	Draw_StretchPic (int x, int y, int w, int h, const char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_TileClear (int x, int y, int w, int h, const char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);

void	Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

void	R_SetPalette ( const unsigned char *palette);
void	R_BeginFrame (void);
void	R_EndFrame (void);

void	GLimp_AppActivate ( bool activate );

#endif
