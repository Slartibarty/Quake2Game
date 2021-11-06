
#include "../../core/core.h"

#define JPH_STAT_COLLECTOR
#define JPH_PROFILE_ENABLED
#define JPH_DEBUG_RENDERER
#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED

#include "../../thirdparty/Jolt/Jolt.h"
#include "../../thirdparty/Jolt/RegisterTypes.h"
#include "../../thirdparty/Jolt/Core/TempAllocator.h"
#include "../../thirdparty/Jolt/Core/JobSystem.h"
#include "../../thirdparty/Jolt/Core/JobSystemThreadPool.h"
#include "../../thirdparty/Jolt/Core/IssueReporting.h"
#include "../../thirdparty/Jolt/Physics/PhysicsSystem.h"
#include "../../thirdparty/Jolt/Physics/PhysicsSettings.h"
#include "../../thirdparty/Jolt/Physics/Body/Body.h"
#include "../../thirdparty/Jolt/Physics/Body/BodyID.h"
#include "../../thirdparty/Jolt/Physics/Body/BodyCreationSettings.h"
#include "../../thirdparty/Jolt/Physics/Collision/Shape/BoxShape.h"
#include "../../thirdparty/Jolt/Physics/Collision/Shape/SphereShape.h"
#include "../../thirdparty/Jolt/Physics/Collision/Shape/CylinderShape.h"

#include "physics_layers.h"
#include "physics.h"

#define MAX_BODIES					10240
#define NUM_BODY_MUTEXES			0			// Autodetect
#define MAX_BODY_PAIRS				65536
#define MAX_CONTACT_CONSTRAINTS		10240

#define NUM_THREADS					0			// How many jobs to run in parallel

namespace PhysicsSystem
{
	static struct physicsLocal_t
	{
		JPH::TempAllocator *	tempAllocator;			// Allocator for temporary allocations
		JPH::JobSystem *		jobSystem;				// The job system that runs physics jobs
		JPH::PhysicsSystem *	physicsSystem;			// The physics system that simulates the world
	} vars;

	static void MyTrace( const char *fmt, ... )
	{
		va_list args;
		char msg[MAX_PRINT_MSG];

		va_start( args, fmt );
		Q_vsprintf_s( msg, fmt, args );
		va_end( args );

		Com_DPrintf( "[JPH] %s\n", msg );
	};

	void Init()
	{
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

		JPH::PhysicsSettings physSettings;

		vars.physicsSystem = new JPH::PhysicsSystem;
		vars.physicsSystem->Init( MAX_BODIES, 0, MAX_BODY_PAIRS, MAX_CONTACT_CONSTRAINTS, GetObjectToBroadPhaseLayer(), BroadPhaseCanCollide, ObjectCanCollide );
		vars.physicsSystem->SetPhysicsSettings( physSettings );
#if defined( JPH_EXTERNAL_PROFILE ) || defined( JPH_PROFILE_ENABLED )
		vars.physicsSystem->SetBroadPhaseLayerToString( GetBroadPhaseLayerName );
#endif
	}

	void Shutdown()
	{
		delete vars.physicsSystem;
		delete vars.jobSystem;
		delete vars.tempAllocator;
	}

	void Frame( float deltaTime )
	{
		vars.physicsSystem->Update( deltaTime, 1, 1, vars.tempAllocator, vars.jobSystem );
	}

	void LoadMap()
	{

	}
}
