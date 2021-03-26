
#pragma once

#include "../shared/engine.h"

#include "ref_public.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

#include "GL/glew.h"
#ifdef _WIN32
#include "GL/wglew.h"
#elif defined __linux__
#include "GL/glxew.h"
#endif

#define REF_VERSION		"GL 0.03"

#define DEFAULT_CLEARCOLOR		0.0f, 0.0f, 0.0f, 1.0f	// Black

#define MAT_EXT ".mat"

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

#define BACKFACE_EPSILON	0.01f

//-------------------------------------------------------------------------------------------------
// gl_misc.cpp
//-------------------------------------------------------------------------------------------------

void GL_ScreenShot_PNG_f();
void GL_ScreenShot_TGA_f();
void GL_Strings_f();

void GL_ExtractWad_f();
void GL_UpgradeWals_f();

// Helpers

void		GL_EnableMultitexture(qboolean enable);
void		GL_SelectTexture(GLenum texture);
// Use this instead of glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
void		GL_TexEnv(GLint value);
// Use this instead of glBindTexture
void		GL_Bind(GLuint texnum);
void		GL_MBind(GLenum target, GLuint texnum);

//-------------------------------------------------------------------------------------------------
// gl_image.cpp
//-------------------------------------------------------------------------------------------------

#define MAX_GLTEXTURES		2048
#define MAX_GLMATERIALS		2048

extern material_t	glmaterials[MAX_GLMATERIALS];
extern int			numglmaterials;

extern byte			g_gammatable[256];

extern material_t	*mat_notexture;
extern material_t	*mat_particletexture;

extern unsigned		d_8to24table[256];

void		GL_ImageList_f(void);
void		GL_MaterialList_f(void);

material_t	*GL_FindMaterial( const char *name, bool managed = false );
material_t	*R_RegisterSkin(const char *name);

void		GL_FreeUnusedMaterials(void);

void		GL_InitImages(void);
void		GL_ShutdownImages(void);

// Image flags
// Mipmaps are opt-out
// Anisotropic filtering is opt-out
// Clamping is opt-in
//
// NOTABLE OVERSIGHT:
// The first material to use an image decides whether an image uses mipmaps or not
// This may confuse artists who expect a mipmapped texture, when the image was first
// referenced without them
// A solution to this problem is to generate mipmaps when required, but only once
//
using imageflags_t = uint8;

#define IF_NONE			0	// Gaben sound pack for FLStudio 6.1
#define IF_NOMIPS		1	// Do not use or generate mipmaps (infers no anisotropy)
#define IF_NOANISO		2	// Do not use anisotropic filtering (only applies to mipmapped images)
#define IF_NEAREST		4	// Use nearest filtering (as opposed to linear filtering)
#define IF_CLAMPS		8	// Clamp to edge (S)
#define IF_CLAMPT		16	// Clamp to edge (T)

struct image_t
{
	char				name[MAX_QPATH];			// game path, including extension
	imageflags_t		flags;
	int					width, height;				// source image
	GLuint				texnum;						// gl texture binding
	int					refcount;
	float				sl, tl, sh, th;				// 0,0 - 1,1 unless part of the scrap
	bool				scrap;						// true if this is part of a larger sheet

	void IncrementRefCount()
	{
		assert( refcount >= 0 );
		++refcount;
	}

	void DecrementRefCount()
	{
		--refcount;
		assert( refcount >= 0 );
	}

	void Delete()
	{
		glDeleteTextures( 1, &texnum );
		memset( this, 0, sizeof( *this ) );
	}
};

struct msurface_t;

struct material_t
{
	char				name[MAX_QPATH];			// game path, including extension
	msurface_t			*texturechain;				// for sort-by-material world drawing
	image_t				*image;						// the only image, may extend to more in the future
	material_t			*nextframe;					// the next frame
	int32				registration_sequence;		// 0 = free, -1 = managed

	// Returns true if this material is the missing texture
	bool IsMissing() { return this == mat_notexture; }

	// Returns true if the image referenced is the missing image
	bool IsImageMissing() { return image == mat_notexture->image; }

	// Returns true if this material is perfectly okay
	bool IsOkay() { return !IsMissing() && !IsImageMissing(); }

	// Bind the referenced texture
	void Bind()
	{
		//assert( image->refcount > 0 );
		GL_Bind( image->texnum );
	}

	// Deference the referenced image and clear this struct
	void Delete()
	{
		image->DecrementRefCount();
		if ( image->refcount == 0 ) {
			// Save time in FreeUnusedImages
			image->Delete();
		}
		memset( this, 0, sizeof( *this ) );
	}
};

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
extern	vec3_t	r_origin; // same as vec3_origin

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

extern	cvar_t	*gl_ext_multitexture;
extern	cvar_t	*gl_ext_pointparameters;
extern	cvar_t	*gl_ext_compiled_vertex_array;

extern	cvar_t	*gl_particle_min_size;
extern	cvar_t	*gl_particle_max_size;
extern	cvar_t	*gl_particle_size;
extern	cvar_t	*gl_particle_att_a;
extern	cvar_t	*gl_particle_att_b;
extern	cvar_t	*gl_particle_att_c;

extern	cvar_t	*gl_mode;
extern	cvar_t	*gl_lightmap;
extern	cvar_t	*gl_shadows;
extern	cvar_t	*gl_dynamic;
extern	cvar_t	*gl_picmip;
extern	cvar_t	*gl_showtris;
extern	cvar_t	*gl_finish;
extern	cvar_t	*gl_clear;
extern	cvar_t	*gl_cullfaces;
extern	cvar_t	*gl_polyblend;
extern	cvar_t	*gl_flashblend;
extern	cvar_t	*gl_modulate;
extern	cvar_t	*gl_swapinterval;
extern  cvar_t  *gl_overbright;
extern  cvar_t  *gl_lockpvs;

extern	cvar_t	*vid_fullscreen;
extern	cvar_t	*vid_gamma;

extern	float	r_world_matrix[16];

extern	model_t	*r_worldmodel;

extern	double	gldepthmin, gldepthmax;

bool 	R_Init(void *hinstance, void *hWnd);
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
void	R_DrawStudioModel (entity_t *e);
void	R_DrawBrushModel (entity_t *e);
void	R_DrawSpriteModel (entity_t *e);
void	R_DrawBeam( entity_t *e );
void	R_DrawWorld (void);
void	R_DrawAlphaSurfaces (void);
void	R_RenderBrushPoly (msurface_t *fa);
void	GL_SubdivideSurface (msurface_t *fa);
bool	R_CullBox (vec3_t mins, vec3_t maxs);
void	R_RotateForEntity (entity_t *e);
void	R_MarkLeaves (void);

void	EmitWaterPolys (msurface_t *fa);
void	R_AddSkySurface (msurface_t *fa);
void	R_ClearSkyBox (void);
void	R_DrawSkyBox (void);

//-------------------------------------------------------------------------------------------------
// gl_draw.cpp
//-------------------------------------------------------------------------------------------------

void	Draw_Init();
void	Draw_Shutdown();

void	Draw_GetPicSize( int *w, int *h, const char *name );
void	Draw_Pic( int x, int y, const char *name );
void	Draw_StretchPic( int x, int y, int w, int h, const char *name );
void	Draw_Char( int x, int y, int ch );
void	Draw_TileClear( int x, int y, int w, int h, const char *name );
void	Draw_Fill( int x, int y, int w, int h, int c );
void	Draw_FadeScreen( void );

void	Draw_RenderBatches();

void	Draw_StretchRaw( int x, int y, int w, int h, int cols, int rows, byte *data );

//-------------------------------------------------------------------------------------------------
// gl_shader.cpp
//-------------------------------------------------------------------------------------------------

struct glProgs_t
{
	GLuint guiProg;
};

extern glProgs_t glProgs;

void	Shaders_Init();
void	Shaders_Shutdown();

//-------------------------------------------------------------------------------------------------

struct glconfig_t
{
	int unused;
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
// Implementation specific functions
//-------------------------------------------------------------------------------------------------

void		GLimp_BeginFrame( void );
void		GLimp_EndFrame( void );

void		GLimp_SetGamma( byte *red, byte *green, byte *blue );
void		GLimp_RestoreGamma( void );

bool 		GLimp_Init( void *hinstance, void *hWnd );
void		GLimp_Shutdown( void );

rserr_t    	GLimp_SetMode( int *pWidth, int *pHeight, int mode, bool fullscreen );

void		GLimp_AppActivate( bool active );

//-------------------------------------------------------------------------------------------------
// Use defines for all imported functions
// This allows us to use the same names for both static links and dynamic links
// We could also use forceinline functions
//-------------------------------------------------------------------------------------------------

// Client functions
extern bool VID_GetModeInfo( int *width, int *height, int mode );
extern void VID_NewWindow( int width, int height );
