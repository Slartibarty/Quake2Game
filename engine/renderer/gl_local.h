
#pragma once

// we are part of the client, but we opt to recieve input from it via refdef_t instead
#include "../shared/engine.h"

#include "ref_types.h"
#include "ref_public.h"

// slart: have to include it this way otherwise it uses the winsdk version...
#include "Inc/DirectXMath.h"
static_assert( DIRECTX_MATH_VERSION >= 316 );

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

#include "GL/glew.h"
#ifdef _WIN32
#include "GL/wglew.h"
#endif

#define DEFAULT_CLEARCOLOR		0.0f, 0.0f, 0.0f, 1.0f	// Black

#define MAT_EXT ".mat"

//=============================================================================

#include "gl_model.h"

#define BACKFACE_EPSILON	0.01f

/*
===============================================================================

	gl_misc.cpp

===============================================================================
*/

// Screenshots

void GL_Screenshot_PNG_f();
void GL_Screenshot_TGA_f();

// Console commands

void R_ExtractWad_f();
void R_UpgradeWals_f();

// OpenGL state helpers

void		GL_EnableMultitexture( bool enable );
void		GL_SelectTexture( GLenum texture );
void		GL_TexEnv( GLint value );
void		GL_Bind( GLuint texnum );
void		GL_MBind( GLenum target, GLuint texnum );

void		GL_CheckErrors();

/*
===============================================================================

	gl_init.cpp

===============================================================================
*/

struct vidDef_t
{
	int	width, height;
};

// GLEW covers all our bases so this is empty right now
struct glConfig_t
{
	int unused;
};

struct glState_t
{
	bool	fullscreen;

	int     prev_mode;

	int		lightmap_textures;

	GLuint	currenttextures[2];
	int		currenttmu;
};

struct renderSystemGlobals_t
{
	refdef_t	refdef;
	int			registrationSequence;

	entity_t *	pViewmodelEntity;		// so we can render this particular entity last after everything else, but before the UI

	DirectX::XMFLOAT4X4A projMatrix;		// the projection matrix
	DirectX::XMFLOAT4X4A viewMatrix;		// the view matrix

	GLuint debugMeshVAO;
	GLuint debugMeshVBO;
};

extern glState_t				glState;
extern glConfig_t				glConfig;
extern renderSystemGlobals_t	tr;			// named "tr" to ease porting from other branches
extern vidDef_t					vid;		// should incorporate this elsewhere eventually, written to by glimp

extern	model_t *r_worldmodel;

extern	double	gldepthmin, gldepthmax;

//
// cvars
//
extern cvar_t *r_norefresh;
extern cvar_t *r_drawentities;
extern cvar_t *r_drawworld;
extern cvar_t *r_drawlights;
extern cvar_t *r_speeds;
extern cvar_t *r_fullbright;
extern cvar_t *r_novis;
extern cvar_t *r_nocull;
extern cvar_t *r_lerpmodels;
extern cvar_t *r_lefthand;

// FIXME: This is a HACK to get the client's light level
extern cvar_t *r_lightlevel;

extern cvar_t *r_vertex_arrays;

extern cvar_t *r_ext_multitexture;
extern cvar_t *r_ext_compiled_vertex_array;

extern cvar_t *r_lightmap;
extern cvar_t *r_shadows;
extern cvar_t *r_mode;
extern cvar_t *r_dynamic;
extern cvar_t *r_modulate;
extern cvar_t *r_picmip;
extern cvar_t *r_showtris;
extern cvar_t *r_wireframe;
extern cvar_t *r_finish;
extern cvar_t *r_clear;
extern cvar_t *r_cullfaces;
extern cvar_t *r_polyblend;
extern cvar_t *r_flashblend;
extern cvar_t *r_overbright;
extern cvar_t *r_swapinterval;
extern cvar_t *r_lockpvs;

extern cvar_t *r_fullscreen;
extern cvar_t *r_gamma;

extern cvar_t *r_basemaps;
extern cvar_t *r_specmaps;
extern cvar_t *r_normmaps;
extern cvar_t *r_emitmaps;

extern cvar_t *r_viewmodelfov;

/*
===============================================================================

	gl_main.cpp

===============================================================================
*/

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
extern	int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

// needed by gl_init
void Particles_Init();
void Particles_Shutdown();

/*
===============================================================================

	gl_image.cpp

===============================================================================
*/

struct image_t;

#define MAX_GLTEXTURES		2048
#define MAX_GLMATERIALS		2048

extern material_t		glmaterials[MAX_GLMATERIALS];
extern int				numglmaterials;

extern byte				g_gammatable[256];

extern material_t *		defaultMaterial;
extern material_t *		blackMaterial;
extern material_t *		whiteMaterial;
extern image_t *		flatNormalImage;

extern unsigned			d_8to24table[256];

void		GL_ImageList_f();
void		GL_MaterialList_f();

material_t	*GL_FindMaterial( const char *name, bool managed = false );

void		GL_FreeUnusedMaterials();

void		GL_InitImages();
void		GL_ShutdownImages();

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
using imageFlags_t = uint8;

#define IF_NONE			0	// Gaben sound pack for FLStudio 6.1
#define IF_NOMIPS		1	// Do not use or generate mipmaps (infers no anisotropy)
#define IF_NOANISO		2	// Do not use anisotropic filtering (only applies to mipmapped images)
#define IF_NEAREST		4	// Use nearest filtering (as opposed to linear filtering)
#define IF_CLAMPS		8	// Clamp to edge (S)
#define IF_CLAMPT		16	// Clamp to edge (T)

struct image_t
{
	char				name[MAX_QPATH];			// game path, including extension
	imageFlags_t		flags;
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
	msurface_t *		texturechain;				// for sort-by-material world drawing
	image_t *			image;						// the diffuse map
	image_t *			specImage;					// the specular map (defaults to black)
	image_t *			normImage;					// the normal map (defaults to funny blue)
	image_t *			emitImage;					// the emission map (defaults to black)
	material_t *		nextframe;					// the next frame
	uint32				alpha;						// alpha transparency, in range 0 - 255
	int32				registration_sequence;		// 0 = free, -1 = managed

	// Returns true if this material is the missing texture
	bool IsMissing() const { return this == defaultMaterial; }

	// Returns true if the image referenced is the missing image
	bool IsImageMissing() const { return image == defaultMaterial->image; }

	// Returns true if this material is perfectly okay
	bool IsOkay() const { return !IsMissing() && !IsImageMissing(); }

	void Register() { registration_sequence = tr.registrationSequence; }

	// Bind the referenced image
	void Bind() const {
		assert( image->refcount > 0 );
		GL_Bind( r_basemaps->GetBool() ? image->texnum : whiteMaterial->image->texnum );
	}

	// Bind the spec image
	void BindSpec() const {
		assert( specImage->refcount > 0 );
		GL_Bind( r_specmaps->GetBool() ? specImage->texnum : blackMaterial->image->texnum );
	}

	// Bind the norm image
	void BindNorm() const {
		assert( normImage->refcount > 0 );
		GL_Bind( r_normmaps->GetBool() ? normImage->texnum : flatNormalImage->texnum );
	}

	// Bind the emission image
	void BindEmit() const {
		assert( emitImage->refcount > 0 );
		GL_Bind( r_emitmaps->GetBool() ? emitImage->texnum : blackMaterial->image->texnum );
	}

	// Deference the referenced image and clear this struct
	void Delete() {
		image->DecrementRefCount();
		if ( image->refcount == 0 ) {
			// Save time in FreeUnusedImages
			image->Delete();
		}
		specImage->DecrementRefCount();
		if ( specImage->refcount == 0 ) {
			specImage->Delete();
		}
		normImage->DecrementRefCount();
		if ( normImage->refcount == 0 ) {
			normImage->Delete();
		}
		emitImage->DecrementRefCount();
		if ( emitImage->refcount == 0 ) {
			emitImage->Delete();
		}
		memset( this, 0, sizeof( *this ) );
	}
};

/*
===============================================================================

	gl_light.cpp

===============================================================================
*/

void	R_MarkLights( dlight_t *light, int bit, mnode_t *node );
void	R_PushDlights();
void	R_LightPoint( vec3_t p, vec3_t color );

/*
===============================================================================

	gl_surf.cpp

===============================================================================
*/

extern int c_visible_lightmaps;
extern int c_visible_textures;

void	R_DrawBrushModel( entity_t *e );
void	R_DrawWorld();
void	R_DrawAlphaSurfaces();
void	R_RenderBrushPoly( msurface_t *fa );
void	R_RotateForEntity( entity_t *e );
void	R_MarkLeaves();

/*
===============================================================================

	gl_mesh.cpp

===============================================================================
*/

void	R_DrawAliasModel( entity_t *e );
void	R_DrawStudioModel( entity_t *e );
void	R_DrawStaticMeshFile( entity_t *e );

/*
===============================================================================

	gl_draw.cpp

===============================================================================
*/

void	Draw_Init();
void	Draw_Shutdown();

void	R_DrawScreenOverlay( const vec4_t color );

void	Draw_RenderBatches();

/*
===============================================================================

	gl_warp.cpp

===============================================================================
*/

void	GL_SubdivideSurface( msurface_t *fa );

void	EmitWaterPolys( msurface_t *fa );
void	R_AddSkySurface( msurface_t *fa );
void	R_ClearSkyBox();
void	R_DrawSkyBox();

/*
===============================================================================

	gl_shader.cpp

===============================================================================
*/

struct glProgs_t
{
	GLuint guiProg;
	GLuint particleProg;
	GLuint smfMeshProg;
	GLuint debugMeshProg;
};

extern glProgs_t glProgs;

void	Shaders_Init();
void	Shaders_Shutdown();

/*
===============================================================================

	Implementation specific functions

	glimp_win.cpp

===============================================================================
*/

void	GLimp_BeginFrame();
void	GLimp_EndFrame();

bool 	GLimp_Init();
void	GLimp_Shutdown();

bool    GLimp_SetMode( int &width, int &height, int mode, bool fullscreen );

void	GLimp_AppActivate( bool active );

/*
===============================================================================

	Functions needed from the main engine

===============================================================================
*/

// Client functions
bool	Sys_GetVidModeInfo( int &width, int &height, int mode );
void	VID_NewWindow( int width, int height );
