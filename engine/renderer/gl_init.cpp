
#include "gl_local.h"

glState_t				glState;
glConfig_t				glConfig;
renderSystemGlobals_t	tr;
vidDef_t				vid;

cvar_t *r_norefresh;
cvar_t *r_drawentities;
cvar_t *r_drawworld;
cvar_t *r_speeds;
cvar_t *r_fullbright;
cvar_t *r_novis;
cvar_t *r_nocull;
cvar_t *r_lerpmodels;
cvar_t *r_lefthand;

// FIXME: This is a HACK to get the client's light level
cvar_t *r_lightlevel;

cvar_t *r_vertex_arrays;

cvar_t *r_ext_multitexture;
cvar_t *r_ext_compiled_vertex_array;

cvar_t *r_lightmap;
cvar_t *r_shadows;
cvar_t *r_mode;
cvar_t *r_dynamic;
cvar_t *r_modulate;
cvar_t *r_picmip;
cvar_t *r_showtris;
cvar_t *r_wireframe;
cvar_t *r_finish;
cvar_t *r_clear;
cvar_t *r_cullfaces;
cvar_t *r_polyblend;
cvar_t *r_flashblend;
cvar_t *r_overbright;
cvar_t *r_swapinterval;
cvar_t *r_lockpvs;

cvar_t *r_fullscreen;
cvar_t *r_gamma;

#ifdef Q_DEBUG

static void GLAPIENTRY GL_DebugProc( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam )
{
	if ( severity == GL_DEBUG_SEVERITY_NOTIFICATION )
	{
		// ignore notifications
		return;
	}

	const char *pSource;
	const char *pType;
	const char *pSeverity;

	switch ( source )
	{
	case GL_DEBUG_SOURCE_API:
		pSource = "API";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		pSource = "WINDOW SYSTEM";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		pSource = "SHADER COMPILER";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		pSource = "THIRD PARTY";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		pSource = "APPLICATION";
		break;
	default: // GL_DEBUG_SOURCE_OTHER
		pSource = "UNKNOWN";
		break;
	}

	switch ( type )
	{
	case GL_DEBUG_TYPE_ERROR:
		pType = "ERROR";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		pType = "DEPRECATED BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		pType = "UDEFINED BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		pType = "PORTABILITY";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		pType = "PERFORMANCE";
		break;
	case GL_DEBUG_TYPE_OTHER:
		pType = "OTHER";
		break;
	case GL_DEBUG_TYPE_MARKER:
		pType = "MARKER";
		break;
	default:
		pType = "UNKNOWN";
		break;
	}

	switch ( severity )
	{
	case GL_DEBUG_SEVERITY_HIGH:
		pSeverity = "HIGH";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		pSeverity = "MEDIUM";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		pSeverity = "LOW";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		pSeverity = "NOTIFICATION";
		break;
	default:
		pSeverity = "UNKNOWN";
		break;
	}

	Com_Printf( "%d: %s of %s severity, raised from %s: %s\n", id, pType, pSeverity, pSource, message );
}

#endif

/*
========================
GL_SetDefaultState

Sets some OpenGL state variables
Called only once at init
========================
*/
void GL_SetDefaultState()
{
	glClearColor( DEFAULT_CLEARCOLOR );
	glCullFace( GL_BACK );

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	glShadeModel( GL_FLAT );

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	GL_TexEnv( GL_REPLACE );

	glEnable( GL_PROGRAM_POINT_SIZE );

#ifdef Q_DEBUG

	glEnable( GL_DEBUG_OUTPUT );
	glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
	glDebugMessageCallback( GL_DebugProc, nullptr );

#endif
}

/*
========================
R_Restart
========================
*/
void R_Restart()
{
	Com_Print( "------- Restarting RenderSystem --------\n" );

	// shutdown everything excluding glimp

	Particles_Shutdown();
	Draw_Shutdown();
	Mod_FreeAll();

	//GLimp_RestoreGamma();
	GL_ShutdownImages();

	Shaders_Shutdown();

	GL_CheckErrors();

	// re-intialise everything (excluding glimp)

	// set a "safe" display mode
	glState.prev_mode = 3;

	// set some default state variables
	GL_SetDefaultState();

	Shaders_Init();

	tr.registrationSequence = 1;

	GL_InitImages();
	//GLimp_SetGamma( g_gammatable, g_gammatable, g_gammatable );

	Mod_Init();
	Draw_Init();
	Particles_Init();

	GL_CheckErrors();

	Com_Print( "RenderSystem restarted\n" "----------------------------------------\n" );
}

/*
========================
R_InitCVars
========================
*/
static void R_InitCVars()
{
	r_lefthand = Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	r_norefresh = Cvar_Get( "r_norefresh", "0", 0 );
	r_fullbright = Cvar_Get( "r_fullbright", "0", 0 );
	r_drawentities = Cvar_Get( "r_drawentities", "1", 0 );
	r_drawworld = Cvar_Get( "r_drawworld", "1", 0 );
	r_novis = Cvar_Get( "r_novis", "0", 0 );
	r_nocull = Cvar_Get( "r_nocull", "0", 0 );
	r_lerpmodels = Cvar_Get( "r_lerpmodels", "1", 0 );
	r_speeds = Cvar_Get( "r_speeds", "0", 0 );

	r_lightlevel = Cvar_Get( "r_lightlevel", "0", 0 );

	r_modulate = Cvar_Get( "r_modulate", "1", CVAR_ARCHIVE );
	r_mode = Cvar_Get( "r_mode", "3", CVAR_ARCHIVE );
	r_lightmap = Cvar_Get( "r_lightmap", "0", 0 );
	r_shadows = Cvar_Get( "r_shadows", "0", CVAR_ARCHIVE );
	r_dynamic = Cvar_Get( "r_dynamic", "1", 0 );
	r_picmip = Cvar_Get( "r_picmip", "0", 0 );
	r_showtris = Cvar_Get( "r_showtris", "0", 0 );
	r_wireframe = Cvar_Get( "r_wireframe", "0", 0 );
	r_finish = Cvar_Get( "r_finish", "0", CVAR_ARCHIVE );
	r_clear = Cvar_Get( "r_clear", "1", 0 );
	r_cullfaces = Cvar_Get( "r_cullfaces", "1", 0 );
	r_polyblend = Cvar_Get( "r_polyblend", "1", 0 );
	r_flashblend = Cvar_Get( "r_flashblend", "0", 0 );
	r_lockpvs = Cvar_Get( "r_lockpvs", "0", 0 );

	r_vertex_arrays = Cvar_Get( "r_vertex_arrays", "0", CVAR_ARCHIVE );

	r_ext_multitexture = Cvar_Get( "r_ext_multitexture", "0", CVAR_ARCHIVE );
	r_ext_compiled_vertex_array = Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE );

	r_overbright = Cvar_Get( "r_overbright", "1", 0 );
	r_swapinterval = Cvar_Get( "r_swapinterval", "0", CVAR_ARCHIVE );

	r_fullscreen = Cvar_Get( "r_fullscreen", "0", CVAR_ARCHIVE );
	r_gamma = Cvar_Get( "r_gamma", "1", CVAR_ARCHIVE );
}

/*
========================
R_InitCommands
========================
*/
static void R_InitCommands()
{
	Cmd_AddCommand( "imagelist", GL_ImageList_f );
	Cmd_AddCommand( "materiallist", GL_MaterialList_f );
	Cmd_AddCommand( "screenshot", GL_ScreenShot_PNG_f );
	Cmd_AddCommand( "screenshot_tga", GL_ScreenShot_TGA_f );
	Cmd_AddCommand( "modellist", Mod_Modellist_f );

	Cmd_AddCommand( "extractwad", R_ExtractWad_f );
	Cmd_AddCommand( "upgradewals", R_UpgradeWals_f );
}

/*
========================
R_Init
========================
*/
bool R_Init()
{
	Com_Print( "------- Initializing RenderSystem --------\n" );

	Com_Printf( "Using DirectXMath v%d\n", DIRECTX_MATH_VERSION );

	// init turbsin
	extern float r_turbsin[256];
	for ( int j = 0; j < 256; j++ ) {
		r_turbsin[j] *= 0.5f;
	}

	R_InitCVars();

	R_InitCommands();

	// initialize OS-specific parts of OpenGL
	if ( !GLimp_Init() ) {
		return false;
	}

	GL_CheckErrors();

	Com_Printf( "OpenGL Vendor  : %s\n", glGetString( GL_VENDOR ) );
	Com_Printf( "OpenGL Renderer: %s\n", glGetString( GL_RENDERER ) );
	Com_Printf( "OpenGL Version : %s\n", glGetString( GL_VERSION ) );
	Com_Printf( "OpenGL GLSL    : %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ) );

	// set a "safe" display mode
	glState.prev_mode = 3;

	// set some default state variables
	GL_SetDefaultState();

	Shaders_Init();

	tr.registrationSequence = 1;

	GL_InitImages();
	//GLimp_SetGamma( g_gammatable, g_gammatable, g_gammatable );

	Mod_Init();
	Draw_Init();
	Particles_Init();

	GL_CheckErrors();

	Com_Print( "RenderSystem initialized\n" "------------------------------------------\n" );

	return true;
}

/*
========================
R_Shutdown
========================
*/
void R_Shutdown()
{
	Particles_Shutdown();
	Draw_Shutdown();
	Mod_FreeAll();

	//GLimp_RestoreGamma();
	GL_ShutdownImages();

	Shaders_Shutdown();

	GLimp_Shutdown();
}

/*
========================
R_AppActivate

Simple wrapper
========================
*/
void R_AppActivate( bool active )
{
	GLimp_AppActivate( active );
}
