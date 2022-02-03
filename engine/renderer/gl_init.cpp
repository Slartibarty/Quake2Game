
#include "gl_local.h"

#ifdef Q_DEBUG
#define Q_DEBUG_GL
#endif

glState_t				glState;
glConfig_t				glConfig;
renderSystemGlobals_t	tr;

cvar_t *r_norefresh;
cvar_t *r_drawentities;
cvar_t *r_drawworld;
cvar_t *r_drawlights;
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
cvar_t *r_nodebug;

cvar_t *r_fullscreen;
cvar_t *r_gamma;
cvar_t *r_multisamples;

cvar_t *r_basemaps;
cvar_t *r_specmaps;
cvar_t *r_normmaps;
cvar_t *r_emitmaps;

cvar_t *r_viewmodelfov;

#ifdef Q_DEBUG_GL

static void GLAPIENTRY GL_DebugProc( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam )
{
	if ( severity == GL_DEBUG_SEVERITY_NOTIFICATION || r_nodebug->GetBool() )
	{
		return;
	}

	const char *pColor;
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
		pType = "UNDEFINED BEHAVIOR";
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
		pColor = S_COLOR_RED;
		pSeverity = "HIGH";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		pColor = S_COLOR_YELLOW;
		pSeverity = "MEDIUM";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		pColor = S_COLOR_YELLOW;
		pSeverity = "LOW";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: // We skip notifications so this case never fires
		pColor = S_COLOR_DEFAULT;
		pSeverity = "NOTIFICATION";
		break;
	default:
		pColor = S_COLOR_DEFAULT;
		pSeverity = "UNKNOWN";
		break;
	}

	Com_Printf( "%s%d: %s of %s severity, raised from %s: %s\n", pColor, id, pType, pSeverity, pSource, message );
}

#endif

/*
========================
R_InitGLState

Sets some OpenGL state variables
Called only once at init
========================
*/
void R_InitGLState()
{
	glClearColor( DEFAULT_CLEARCOLOR );
	glFrontFace( GL_CW );

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	GL_TexEnv( GL_REPLACE );

	glEnable( GL_PROGRAM_POINT_SIZE );
	//glEnable( GL_FRAMEBUFFER_SRGB );

#ifdef Q_DEBUG_GL

	if ( GLEW_KHR_debug )
	{
		glEnable( GL_DEBUG_OUTPUT );
		glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
		glDebugMessageCallback( GL_DebugProc, nullptr );
	}

#endif
}

/*
========================
R_InitCVars
========================
*/
static void R_InitCVars()
{
	r_lefthand = Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE, "Determines weapon handedness." );
	r_norefresh = Cvar_Get( "r_norefresh", "0", 0, "Disables rendering, you won't be able to see the console." );
	r_fullbright = Cvar_Get( "r_fullbright", "0", 0, "If true, lighting is ignored." );
	r_drawentities = Cvar_Get( "r_drawentities", "1", 0, "If true, entities are drawn." );
	r_drawworld = Cvar_Get( "r_drawworld", "1", 0, "If true, the world is drawn." );
	r_drawlights = Cvar_Get( "r_drawlights", "0", 0, "If true, lights are drawn." );
	r_novis = Cvar_Get( "r_novis", "0", 0, "If true, vis is ignored." );
	r_nocull = Cvar_Get( "r_nocull", "0", 0, "If true, world polygons are not frustrum culled." );
	r_lerpmodels = Cvar_Get( "r_lerpmodels", "1", 0, "If true, md2 models are vertex lerped." );
	r_speeds = Cvar_Get( "r_speeds", "0", 0, "If true, perf info is printed to the console every frame.");

	r_lightlevel = Cvar_Get( "r_lightlevel", "0", 0, "A terrible hack to determine the client's light level." );

	r_modulate = Cvar_Get( "r_modulate", "1", CVAR_ARCHIVE, "Deprecated." );
	r_mode = Cvar_Get( "r_mode", "3", CVAR_ARCHIVE, "The video mode." );
	r_lightmap = Cvar_Get( "r_lightmap", "0", 0, "If true, displays the lightmap." );
	r_shadows = Cvar_Get( "r_shadows", "0", CVAR_ARCHIVE, "Deprecated." );
	r_dynamic = Cvar_Get( "r_dynamic", "0", 0, "If true, dynamic lighting is enabled." );
	r_picmip = Cvar_Get( "r_picmip", "0", 0, "Deprecated." );
	r_showtris = Cvar_Get( "r_showtris", "0", 0, "Deprecated." );
	r_wireframe = Cvar_Get( "r_wireframe", "0", 0, "If true, draws everything using lines." );
	r_finish = Cvar_Get( "r_finish", "0", CVAR_ARCHIVE, "Deprecated." );
	r_clear = Cvar_Get( "r_clear", "1", 0, "If true, clears the framebuffer at the start of each frame." );
	r_cullfaces = Cvar_Get( "r_cullfaces", "1", 0, "If true, back-facing polygons are culled." );
	r_polyblend = Cvar_Get( "r_polyblend", "1", 0, "If true, displays screen flashes." );
	r_flashblend = Cvar_Get( "r_flashblend", "0", 0, "Deprecated." );
	r_lockpvs = Cvar_Get( "r_lockpvs", "0", 0, "Locks the active PVS." );
	r_nodebug = Cvar_Get( "r_nodebug", "0", 0, "Disable OpenGL debug spew." );

	r_vertex_arrays = Cvar_Get( "r_vertex_arrays", "0", CVAR_ARCHIVE, "Deprecated.");

	r_ext_multitexture = Cvar_Get( "r_ext_multitexture", "0", CVAR_ARCHIVE, "MD2 Deprecated." );
	r_ext_compiled_vertex_array = Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE, "MD2 Deprecated." );

	r_overbright = Cvar_Get( "r_overbright", "1", 0, "Whether to use better blending for lightmaps. Deprecated.");
	r_swapinterval = Cvar_Get( "r_swapinterval", "0", CVAR_ARCHIVE, "Determines the page flip interval, -1 = adaptive vsync, 0 = immediate, 1 = vsync" );

	r_fullscreen = Cvar_Get( "r_fullscreen", "0", CVAR_ARCHIVE, "If true, the game draws in a fullscreen window." );
	r_gamma = Cvar_Get( "r_gamma", "1", CVAR_ARCHIVE, "Deprecated." );
	r_multisamples = Cvar_Get( "r_multisamples", "8", CVAR_ARCHIVE, "Number of times to multisample pixels. Valid values are powers of 2 between 0 and 8." );

	r_basemaps = Cvar_Get( "r_basemaps", "1", 0, "Whether to bind diffuse maps." );
	r_specmaps = Cvar_Get( "r_specmaps", "1", 0, "Whether to bind specular maps." );
	r_normmaps = Cvar_Get( "r_normmaps", "1", 0, "Whether to bind normal maps." );
	r_emitmaps = Cvar_Get( "r_emitmaps", "1", 0, "Whether to bind emission maps." );

	r_viewmodelfov = Cvar_Get( "r_viewmodelfov", "42", 0, "The field of view to draw viewmodels at.");
}

/*
========================
R_InitCommands
========================
*/
static void R_InitCommands()
{
	Cmd_AddCommand( "r_imageList", GL_ImageList_f, "Lists all registered images." );
	Cmd_AddCommand( "r_materialList", GL_MaterialList_f, "Lists all registered materials." );
	Cmd_AddCommand( "r_modelList", Mod_Modellist_f, "Lists all registered models." );
	Cmd_AddCommand( "screenshot", GL_Screenshot_PNG_f, "Takes a PNG screenshot." );
	Cmd_AddCommand( "screenshot_tga", GL_Screenshot_TGA_f, "Takes a TGA screenshot." );

	Cmd_AddCommand( "r_extractWad", R_ExtractWad_f, "Extracts a Quake 1 WAD to the textures directory." );
	Cmd_AddCommand( "r_upgradeWals", R_UpgradeWals_f, "Converts a folder of Quake 2 WAL files to PNG." );
}

/*
========================
R_Init
========================
*/
static void R_CreateDebugMesh()
{
#if 1
	glGenVertexArrays( 1, &tr.debugMeshVAO );
	glGenBuffers( 1, &tr.debugMeshVBO );

	glBindVertexArray( tr.debugMeshVAO );
	glBindBuffer( GL_ARRAY_BUFFER, tr.debugMeshVBO );

	glEnableVertexAttribArray( 0 );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( float[3] ), (void *)( 0 ) );

	int i = 0, j = 1;
	vec3 vertices[11];

	vertices[0].Set( 0.0f, 0.0f, -16.0f );
	for ( ; i <= 4; ++i, ++j ) {
		vertices[j].Set( 16.0f*cos(i*M_PI_F/2.0f), 16.0f*sin(i*M_PI_F/2.0f), 0.0f );
	}

	vertices[j].Set( 0.0f, 0.0f, 16.0f );
	for ( i = 4; i >= 0; --i, ++j ) {
		vertices[j].Set( -16.0f*cos(i*M_PI_F/2.0f), -16.0f*sin(i*M_PI_F/2.0f), 0.0f );
	}

	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );
#endif
}

/*
========================
R_DestroyDebugMesh
========================
*/
static void R_DestroyDebugMesh()
{
#if 1
	glDeleteBuffers( 1, &tr.debugMeshVBO );
	glDeleteVertexArrays( 1, &tr.debugMeshVAO );
#endif
}

/*
========================
R_Restart
========================
*/
void R_Restart()
{
	Com_Print( "-------- Restarting RenderSystem --------\n" );

	// shutdown everything excluding glimp

	Particles_Shutdown();
	Draw_Shutdown();
	Mod_FreeAll();

	GL_ShutdownImages();

	R_DestroyDebugMesh();

	Shaders_Shutdown();

	GL_CheckErrors();

	GLimp_Shutdown();

	// re-intialise everything

	if ( !GLimp_Init() ) {
		Com_FatalError( "Failed to re-initialise GLimp after a vid_restart!\n" );
	}

	// set a "safe" display mode
	glState.prev_mode = 3;

	// set some default state variables
	R_InitGLState();

	Shaders_Init();

	tr.registrationSequence = 1;

	GL_InitImages();

	Mod_Init();
	Draw_Init();
	Particles_Init();

	GL_CheckErrors();

	Com_Print( "RenderSystem restarted\n" "-----------------------------------------\n\n" );
}

/*
========================
R_Init
========================
*/
bool R_Init()
{
	Com_Print( "-------- Initializing RenderSystem --------\n" );

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

	if ( !GLEW_ARB_framebuffer_object ) {
		// need an error message solution, just a const char *& in the params maybe?
		return false;
	}
	// BC7, BC6H, BC5 and BC4
	if ( !GLEW_ARB_texture_compression_bptc || !GLEW_ARB_texture_compression_rgtc ) {
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
	R_InitGLState();

	Shaders_Init();

	tr.registrationSequence = 1;

	R_CreateDebugMesh();

	GL_InitImages();

	Mod_Init();
	Draw_Init();
	Particles_Init();

	GL_CheckErrors();

	Com_Print( "RenderSystem initialized\n" "-------------------------------------------\n\n" );

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

	GL_ShutdownImages();

	R_DestroyDebugMesh();

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
