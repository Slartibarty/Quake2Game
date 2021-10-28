
#include "../../core/core.h"

#include "../../framework/framework_public.h"

#ifdef _WIN32
#include <process.h>
#include "../../core/sys_includes.h"
#include <ShlObj.h>
#endif

#include "GL/glew.h"
#include "GL/wglew.h"

#include "../../framework/renderer/gl_dummywindow.h"

#include "SDL2/include/SDL.h"

#include "Jolt/Jolt.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Core/JobSystem.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/PhysicsSettings.h"
#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/Physics/Collision/Shape/CylinderShape.h"

#include "layers.h"
#include "debugrenderer.h"

#define WINDOW_WIDTH				1920
#define WINDOW_HEIGHT				1080

#define MAX_BODIES					10240
#define NUM_BODY_MUTEXES			0			// Autodetect
#define MAX_BODY_PAIRS				65536
#define MAX_CONTACT_CONSTRAINTS		10240

#define NUM_THREADS					0			// How many jobs to run in parallel

static struct collisionTestLocal_t
{
	JPH::TempAllocator *	tempAllocator;			// Allocator for temporary allocations
	JPH::JobSystem *		jobSystem;				// The job system that runs physics jobs
	JPH::PhysicsSystem *	physicsSystem;			// The physics system that simulates the world
	MyDebugRenderer *		debugRenderer;			// Renders things
	//JPH::ContactListenerImpl *contactListener;	// Contact listener implementation

	SDL_Window *			window;
} vars;

static void MyTrace( const char *fmt, ... )
{
	va_list args;
	char msg[MAX_PRINT_MSG];

	va_start( args, fmt );
	Q_vsprintf_s( msg, fmt, args );
	va_end( args );

	Com_DPrintf( "%s\n", msg );
};

static void CT_Init( int argc, char **argv )
{
	// Add a Windows console
	if ( AllocConsole() == FALSE )
	{
		exit( EXIT_FAILURE );
	}

	// Core
	Time_Init();

	// Framework
	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();

	// Add +set commands
	Cvar_AddEarlyCommands( argc, argv );

	// OVERSIGHT: filesystem doesn't get the luxury of using cvars from cfg files
	FileSystem::Init();

	// Add config files
	Cbuf_AddText( "exec collisiontest.cfg\n" );
	Cbuf_Execute();

	// Re-add +set commands to override config files
	Cvar_AddEarlyCommands( argc, argv );

	// Add + commands
	Cbuf_AddLateCommands( argc, argv );

	// Init SDL
	if ( SDL_InitSubSystem( SDL_INIT_EVERYTHING ) != 0 )
	{
		exit( EXIT_FAILURE );
	}

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

	// Init OpenGL

	{
		DummyVars dvars;
		WGLimp_CreateDummyWindow( dvars );

		// initialize our OpenGL dynamic bindings
		glewExperimental = GL_TRUE;
		if ( glewInit() != GLEW_OK || wglewInit() != GLEW_OK )
		{
			exit( EXIT_FAILURE );
		}

		WGLimp_DestroyDummyWindow( dvars );
	}

	SDL_Window *window = SDL_CreateWindow( "Ploppers", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL );
	SDL_GLContext context = SDL_GL_CreateContext( window );
	SDL_GL_MakeCurrent( window, context );
	vars.window = window;

	//
	// Init JPH
	//

	JPH::RegisterTypes();
	JPH::Trace = MyTrace;

	// Allocate temp memory
#ifdef JPH_DISABLE_TEMP_ALLOCATOR
	vars.tempAllocator = new JPH::TempAllocatorMalloc();
#else
	vars.tempAllocator = new JPH::TempAllocatorImpl( 16 * 1024 * 1024 );
#endif

	// Create job system
	vars.jobSystem = new JPH::JobSystemThreadPool( JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, NUM_THREADS - 1 );

	vars.debugRenderer = new MyDebugRenderer;

	JPH::PhysicsSettings physSettings;

	vars.physicsSystem = new JPH::PhysicsSystem;
	vars.physicsSystem->Init( MAX_BODIES, 0, MAX_BODY_PAIRS, MAX_CONTACT_CONSTRAINTS, GetObjectToBroadPhaseLayer(), BroadPhaseCanCollide, ObjectCanCollide );
	vars.physicsSystem->SetPhysicsSettings( physSettings );
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	vars.physicsSystem->SetBroadPhaseLayerToString( GetBroadPhaseLayerName );
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED
}

static void CT_Shutdown()
{
	SDL_QuitSubSystem( SDL_INIT_EVERYTHING );
	SDL_Quit();

	FileSystem::Shutdown();
	Cvar_Shutdown();
	Cmd_Shutdown();
	Cbuf_Shutdown();

	FreeConsole();
}

[[noreturn]]
static void CT_Quit( int code )
{
	CT_Shutdown();

	exit( code );
}

//=================================================================================================

#if 0

template<typename T>
struct vec3_base
{
	union
	{
		struct
		{
			T x, y, z;
		};
		T v[3];
	};

	vec3_base() = default;

	vec3_base( const vec3_base & ) = default;
	vec3_base &operator=( const vec3_base & ) = default;

	vec3_base( vec3_base && ) = default;
	vec3_base &operator=( vec3_base && ) = default;

	constexpr vec3_base( T X, T Y, T Z ) noexcept : x( X ), y( Y ), z( Z ) {}

	T Length() const
	{
		return sqrt( x * x + y * y + z * z );
	}

	T Normalize()
	{
		float length = Length();

		// prevent divide by zero
		float ilength = 1.0f / ( length + FLT_EPSILON );

		x *= ilength;
		y *= ilength;
		z *= ilength;

		return length;
	}

	void NormalizeFast()
	{
		// prevent divide by zero
		T ilength = 1.0f / ( Length() + FLT_EPSILON );

		x *= ilength;
		y *= ilength;
		z *= ilength;
	}
};

template<typename T>
inline vec3_base<T> operator+( vec3_base<T> v1, vec3_base<T> v2 )
{
	return vec3_base<T>( v1.x + v2.x, v1.y + v2.y, v1.z + v2.z );
}

template<typename T>
inline vec3_base<T> operator-( vec3_base<T> v1, vec3_base<T> v2 )
{
	return vec3_base<T>( v1.x - v2.x, v1.y - v2.y, v1.z - v2.z );
}

template<typename T>
inline vec3_base<T> operator*( vec3_base<T> v1, vec3_base<T> v2 )
{
	return vec3_base<T>( v1.x * v2.x, v1.y * v2.y, v1.z * v2.z );
}

template<typename T>
inline vec3_base<T> operator/( vec3_base<T> v1, vec3_base<T> v2 )
{
	return vec3_base<T>( v1.x / v2.x, v1.y / v2.y, v1.z / v2.z );
}

template<typename T>
inline vec3_base<T> operator/( vec3_base<T> v1, T val )
{
	return vec3_base<T>( v1.x / val, v1.y / val, v1.z / val );
}

using vec3x = vec3_base<float>;

struct aabb_t
{
	vec3x mins;
	vec3x maxs;

	vec3x GetCenter() const { return ( maxs + mins ) / 2.0f; }
	float GetRadius() const { return ( mins - GetCenter() ).Length(); }
};

static void CT_TestMaths()
{
	vec3x vec1( 756.0f, 87.0f, 423.0f );
	vec3x vec2( 23.0f, 241.0f, 867.0f );

	vec3x test1 = vec2 + vec1;

}

#endif

class MyContactListener final : public JPH::ContactListener
{
public:
	void OnContactAdded( const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings ) override
	{
		Com_Print( "Something impacted!\n" );
	}
};

static void CT_CreateFloor( JPH::BodyInterface &bodyInterface )
{
	const JPH::BodyCreationSettings creationSettings( new JPH::BoxShape( JPH::Vec3( 100.0f, 1.0f, 100.0f ), 0.0f ), JPH::Vec3( 0.0f, -1.0f, 0.0f ), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING );

	JPH::Body *floor = bodyInterface.CreateBody( creationSettings );
#ifdef Q_DEBUG
	floor->SetDebugName( "Floor" );
#endif
	bodyInterface.AddBody( floor->GetID(), JPH::EActivation::DontActivate );
}

static bool CT_HandleEvent( SDL_Event &event )
{
	switch ( event.type )
	{
	case SDL_QUIT:
		return false;
	}

	return true;
}

int main( int argc, char **argv )
{
#ifdef _WIN32
	// Make sure the CRT thinks we're a GUI app, this makes CRT asserts use a message box
	// rather than printing to stderr
	_set_app_type( _crt_gui_app );
#ifdef Q_MEM_DEBUG
	_CrtSetDbgFlag( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF );
#endif
#endif

	int length = Q_sprintf_s( nullptr, 0, "%d %d\n", 15, 25 );

	CT_Init( argc, argv );

	JPH::BodyInterface &bodyInterface = vars.physicsSystem->GetBodyInterface();

	CT_CreateFloor( bodyInterface );

	JPH::Body *sphere1 = bodyInterface.CreateBody( JPH::BodyCreationSettings( new JPH::SphereShape( 16.0f ), JPH::Vec3( 0.0f, 16.0f, 0.0f ), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING ) );
	bodyInterface.AddBody( sphere1->GetID(), JPH::EActivation::Activate );
	JPH::Body *sphere2 = bodyInterface.CreateBody( JPH::BodyCreationSettings( new JPH::CylinderShape( 48.0f, 16.0f ), JPH::Vec3(0.0f, 96.0f, 3.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING));
	bodyInterface.AddBody( sphere2->GetID(), JPH::EActivation::Activate );

	// Install contact listener
	vars.physicsSystem->SetContactListener( new MyContactListener );

	vars.physicsSystem->OptimizeBroadPhase();

	JPH::BodyManager::DrawSettings drawSettings
	{
		//.mDrawGetSupportFunction = true,
		//.mDrawSupportDirection = true,
		//.mDrawGetSupportingFace = true,
		.mDrawShapeWireframe = true,
		.mDrawBoundingBox = true,
		//.mDrawCenterOfMassTransform = true,
		//.mDrawWorldTransform = true,
		.mDrawVelocity = true,
		//.mDrawMassAndInertia = true
	};

	double deltaTime, oldTime, newTime;
	oldTime = Time_FloatSeconds();

	SDL_Event event;
	while ( 1 )
	{
		while ( SDL_PollEvent( &event ) )
		{
			if ( !CT_HandleEvent( event ) )
			{
				CT_Quit( EXIT_SUCCESS );
			}
		}

		newTime = Time_FloatSeconds();
		deltaTime = newTime - oldTime;

		vars.physicsSystem->Update( static_cast<float>( deltaTime ), 1, 1, vars.tempAllocator, vars.jobSystem );

		vars.debugRenderer->BeginFrame( WINDOW_WIDTH, WINDOW_HEIGHT );
		vars.physicsSystem->DrawBodies( drawSettings, vars.debugRenderer );
		vars.debugRenderer->EndFrame( vars.window );

		oldTime = newTime;
	}

	// Unreachable
	return 0;
}

/*
===================================================================================================
	Log implementations
===================================================================================================
*/

[[noreturn]]
static void Sys_Error( const platChar_t *mainInstruction, const char *msg )
{
	// Trim the newline
	char newMsg[MAX_PRINT_MSG];
	Q_strcpy_s( newMsg, msg );
	char *lastNewline = newMsg + strlen( newMsg ) - 1;
	if ( *lastNewline == '\n' )
	{
		*lastNewline = '\0';
	}

	// Use old msg, we need the newline
	Sys_OutputDebugString( msg );

	wchar_t reason[MAX_PRINT_MSG];
	Sys_UTF8ToUTF16( newMsg, Q_strlen( newMsg ) + 1, reason, countof( reason ) );

	const platChar_t *windowTitle = FileSystem::ModInfo::GetWindowTitle();

	TaskDialog(
		nullptr,
		nullptr,
		windowTitle,
		mainInstruction,
		reason,
		TDCBF_OK_BUTTON,
		TD_ERROR_ICON,
		nullptr );

	exit( EXIT_FAILURE );
}

/*
========================
Com_Print

Both client and server can use this, and it will output
to the appropriate place.
========================
*/

static void CopyAndStripColorCodes( char *dest, strlen_t destSize, const char *src )
{
	const char *last;
	int c;

	last = dest + destSize - 1;
	while ( ( dest < last ) && ( c = *src ) != 0 ) {
		if ( src[0] == C_COLOR_ESCAPE && src + 1 != last && IsColorIndex( src[1] ) ) {
			src++;
		}
		else {
			*dest++ = c;
		}
		src++;
	}
	*dest = '\0';
}

void Com_Print( const char *msg )
{
	char newMsg[MAX_PRINT_MSG];

	// create a copy of the msg for places that don't want the colour codes
	CopyAndStripColorCodes( newMsg, sizeof( newMsg ), msg );

	Sys_OutputDebugString( newMsg );

	// echo to the console too
	WriteConsoleA( GetStdHandle( STD_OUTPUT_HANDLE ), newMsg, (DWORD)strlen( newMsg ), nullptr, nullptr );
}

void Com_Printf( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Print( msg );
}

/*
========================
Com_DPrint

A Com_Print that only shows up if the "developer" cvar is set
========================
*/

void Com_DPrint( const char *msg )
{
	Com_Printf( S_COLOR_YELLOW "%s" S_COLOR_RESET, msg );
}

void Com_DPrintf( _Printf_format_string_ const char *fmt, ... )
{
	va_list argptr;
	char msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Printf( S_COLOR_YELLOW "%s" S_COLOR_RESET, msg );
}

/*
========================
Com_Error

The peaceful option
Equivalent to an old Com_Error( ERR_DROP )
========================
*/

[[noreturn]]
void Com_Error( const char *msg )
{
	Com_FatalError( msg );
}

[[noreturn]]
void Com_Errorf( _Printf_format_string_ const char *fmt, ... )
{
	va_list argptr;
	char msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_FatalError( msg );
}

/*
========================
Com_FatalError

The nuclear option
Equivalent to an old Com_Error( ERR_FATAL )
Kills the server, kills the client, shuts the engine down and quits the program
========================
*/

[[noreturn]]
void Com_FatalError( const char *msg )
{
	CT_Shutdown();

	Sys_Error( PLATTEXT( "Engine Error" ), msg );
}

[[noreturn]]
void Com_FatalErrorf( _Printf_format_string_ const char *fmt, ... )
{
	va_list argptr;
	char msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_FatalError( msg );
}
