
#pragma once

#include "../shared/common.h"
#include "../ref_shared/ref_public.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include "GL/glew.h"
#ifdef _WIN32
#include "GL/wglew.h"
#endif

#define REF_VERSION		"GL 0.02"

//-------------------------------------------------------------------------------------------------
// Image types
//-------------------------------------------------------------------------------------------------

enum imagetype_t
{
	it_skin,
	it_sprite,
	it_wall,
	it_pic,
	it_sky
};

struct msurface_t;

struct image_t
{
	char				name[MAX_QPATH];			// game path, including extension
	imagetype_t			type;
	int					width, height;				// source image
	int					registration_sequence;		// 0 = free
	msurface_t			*texturechain;				// for sort-by-texture world drawing
	GLuint				texnum;						// gl texture binding
	float				sl, tl, sh, th;				// 0,0 - 1,1 unless part of the scrap
	bool				scrap;
};

#define TEXNUM_LIGHTMAPS	1024

#define MAX_GLTEXTURES		2048

//-------------------------------------------------------------------------------------------------

// Convenient return type
enum rserr_t
{
	rserr_ok,

	rserr_invalid_fullscreen,
	rserr_invalid_mode,

	rserr_unknown
};

#include "gl_model.h"

// Our vertex format, doesn't appear to be used
struct glvert_t
{
	float	x, y, z;
	float	s, t;
	float	r, g, b;
};

#define	MAX_LBM_HEIGHT		480		// Unused

#define BACKFACE_EPSILON	0.01f

//-------------------------------------------------------------------------------------------------
// gl_misc.cpp
//-------------------------------------------------------------------------------------------------

void GL_ScreenShot_f(void);
void GL_Strings_f(void);

void GL_SetDefaultState(void);

void GL_ExtractWad_f( void );
void GL_UpgradeWals_f( void );

//-------------------------------------------------------------------------------------------------
// gl_image.cpp
//-------------------------------------------------------------------------------------------------

extern image_t		gltextures[MAX_GLTEXTURES];
extern int			numgltextures;

extern byte			g_gammatable[256];

extern image_t		*r_notexture;
extern image_t		*r_particletexture;

extern unsigned		d_8to24table[256];

void		GL_TextureMode(char *string);
void		GL_ImageList_f(void);

image_t		*GL_FindImage(const char *name, imagetype_t type);
image_t		*R_RegisterSkin(const char *name);

void		GL_FreeUnusedImages(void);

void		GL_InitImages(void);
void		GL_ShutdownImages(void);

// Helpers

void		GL_EnableMultitexture(qboolean enable);
void		GL_SelectTexture(GLenum texture);
			// Use this instead of glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
void		GL_TexEnv(GLint value);
			// Use this instead of glBindTexture
void		GL_Bind(GLuint texnum);
void		GL_MBind(GLenum target, GLuint texnum);

//-------------------------------------------------------------------------------------------------
// gl_main.cpp
//-------------------------------------------------------------------------------------------------

struct viddef_t
{
	int	width, height;
};
extern viddef_t vid;

extern	entity_t	*currententity;
extern	model_t		*currentmodel;
extern	int			r_visframecount;
extern	int			r_framecount;
extern	cplane_t	frustum[4];
extern	int			c_brush_polys, c_alias_polys;

//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_newrefdef;
extern	int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern	cvar_t	*r_norefresh;
extern	cvar_t	*r_lefthand;
extern	cvar_t	*r_drawentities;
extern	cvar_t	*r_drawworld;
extern	cvar_t	*r_speeds;
extern	cvar_t	*r_fullbright;
extern	cvar_t	*r_novis;
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_lerpmodels;

extern	cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

extern	cvar_t	*gl_vertex_arrays;

extern	cvar_t	*gl_ext_swapinterval;
extern	cvar_t	*gl_ext_multitexture;
extern	cvar_t	*gl_ext_pointparameters;
extern	cvar_t	*gl_ext_compiled_vertex_array;

extern	cvar_t	*gl_particle_min_size;
extern	cvar_t	*gl_particle_max_size;
extern	cvar_t	*gl_particle_size;
extern	cvar_t	*gl_particle_att_a;
extern	cvar_t	*gl_particle_att_b;
extern	cvar_t	*gl_particle_att_c;

extern	cvar_t	*gl_nosubimage;
extern	cvar_t	*gl_mode;
extern	cvar_t	*gl_lightmap;
extern	cvar_t	*gl_shadows;
extern	cvar_t	*gl_dynamic;
extern	cvar_t	*gl_round_down;
extern	cvar_t	*gl_picmip;
extern	cvar_t	*gl_skymip;
extern	cvar_t	*gl_showtris;
extern	cvar_t	*gl_finish;
extern	cvar_t	*gl_clear;
extern	cvar_t	*gl_cull;
extern	cvar_t	*gl_poly;
extern	cvar_t	*gl_texsort;
extern	cvar_t	*gl_polyblend;
extern	cvar_t	*gl_flashblend;
extern	cvar_t	*gl_lightmaptype;
extern	cvar_t	*gl_modulate;
extern	cvar_t	*gl_playermip;
extern	cvar_t	*gl_3dlabs_broken;
extern	cvar_t	*gl_swapinterval;
extern	cvar_t	*gl_texturemode;
extern  cvar_t  *gl_saturatelighting;
extern  cvar_t  *gl_lockpvs;

extern	cvar_t	*vid_fullscreen;
extern	cvar_t	*vid_gamma;

extern	float	r_world_matrix[16];

extern	model_t	*r_worldmodel;

extern	double	gldepthmin, gldepthmax;

int 	R_Init(void *hinstance, void *hWnd);
void	R_Shutdown(void);

void	R_RenderView(refdef_t *fd);

//-------------------------------------------------------------------------------------------------
// gl_light.cpp
//-------------------------------------------------------------------------------------------------

void	R_RenderDlights(void);

void	R_MarkLights(dlight_t *light, int bit, mnode_t *node);
void	R_PushDlights(void);
void	R_LightPoint(vec3_t p, vec3_t color);

//-------------------------------------------------------------------------------------------------
// gl_surf.cpp
//-------------------------------------------------------------------------------------------------

extern	int		c_visible_lightmaps;
extern	int		c_visible_textures;

void	R_DrawAliasModel (entity_t *e);
void	R_DrawBrushModel (entity_t *e);
void	R_DrawSpriteModel (entity_t *e);
void	R_DrawBeam( entity_t *e );
void	R_DrawWorld (void);
void	R_DrawAlphaSurfaces (void);
void	R_RenderBrushPoly (msurface_t *fa);
void	Draw_InitLocal (void);
void	GL_SubdivideSurface (msurface_t *fa);
qboolean	R_CullBox (vec3_t mins, vec3_t maxs);
void	R_RotateForEntity (entity_t *e);
void	R_MarkLeaves (void);

void	EmitWaterPolys (msurface_t *fa);
void	R_AddSkySurface (msurface_t *fa);
void	R_ClearSkyBox (void);
void	R_DrawSkyBox (void);

//-------------------------------------------------------------------------------------------------
// gl_draw.cpp
//-------------------------------------------------------------------------------------------------

void	Draw_GetPicSize (int *w, int *h, const char *name);
void	Draw_Pic (int x, int y, const char *name);
void	Draw_StretchPic (int x, int y, int w, int h, const char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_TileClear (int x, int y, int w, int h, const char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);
void	Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

//-------------------------------------------------------------------------------------------------
// TODO
//-------------------------------------------------------------------------------------------------

void	R_BeginFrame( void );
void	R_SetPalette ( const unsigned char *palette);

//-------------------------------------------------------------------------------------------------
// GL extension emulation functions
//-------------------------------------------------------------------------------------------------
void GL_DrawParticles( int num_particles, const particle_t particles[], const unsigned colortable[768] );

//-------------------------------------------------------------------------------------------------

struct glconfig_t
{
	const char *renderer_string;
	const char *vendor_string;
	const char *version_string;
	const char *extensions_string;
};

struct glstate_t
{
	bool	fullscreen;

	int     prev_mode;

	int		lightmap_textures;

	GLuint	currenttextures[2];
	int		currenttmu;
};

extern glconfig_t	gl_config;
extern glstate_t	gl_state;

//-------------------------------------------------------------------------------------------------
// Imported functions
//-------------------------------------------------------------------------------------------------

extern refimport_t ri;		// gl_main.cpp

//-------------------------------------------------------------------------------------------------
// Implementation specific functions
//-------------------------------------------------------------------------------------------------

void		GLimp_BeginFrame( void );
void		GLimp_EndFrame( void );

void		GLimp_SetGamma( byte *red, byte *green, byte *blue );
void		GLimp_RestoreGamma( void );

int 		GLimp_Init( void *hinstance, void *hWnd );
void		GLimp_Shutdown( void );

rserr_t    	GLimp_SetMode( int *pWidth, int *pHeight, int mode, bool fullscreen );

void		GLimp_AppActivate( qboolean active );
void		*GLimp_GetWindowHandle( void );
