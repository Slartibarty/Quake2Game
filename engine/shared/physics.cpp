
#if 1

#include "engine.h"

#include <Jolt.h>
#include <RegisterTypes.h>

#include <Core/TempAllocator.h>
#include <Core/JobSystemThreadPool.h>

#include <Physics/PhysicsSettings.h>
#include <Physics/PhysicsSystem.h>

#include <Physics/Collision/CastResult.h>
#include <Physics/Collision/RayCast.h>
#include <Physics/Collision/NarrowPhaseQuery.h>

#include <Physics/Collision/Shape/BoxShape.h>
#include <Physics/Collision/Shape/MeshShape.h>
#include <Physics/Collision/Shape/SphereShape.h>

#include <Physics/Body/BodyCreationSettings.h>
#include <Physics/Body/BodyActivationListener.h>

#include "physics.h"

// If this changes, our type has to change too :(
static_assert( sizeof( JPH::BodyID ) == sizeof( uint32 ) );

namespace Physics
{

//=================================================================================================

// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
static constexpr uint MAX_BODIES = 1024;

// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
static constexpr uint NUM_BODY_MUTEXES = 0;

// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
static constexpr uint MAX_BODY_PAIRS = 1024;

// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
static constexpr uint MAX_CONTACT_CONSTRAINTS = 1024;

static constexpr int NUM_THREADS = 0; // Autodetect

//=================================================================================================

static void MyTrace( const char *fmt, ... )
{
	va_list args;
	char msg[MAX_PRINT_MSG];

	va_start( args, fmt );
	Q_vsprintf_s( msg, fmt, args );
	va_end( args );

	Com_Printf( "[JPH] %s\n", msg );
};

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool MyAssertFailed( const char *inExpression, const char *inMessage, const char *inFile, uint inLine )
{
	Com_Printf( "[JPH ASSERT] %s:%u: (%s) %s\n", inFile, inLine, inExpression, inMessage != nullptr ? inMessage : "" );

	// Breakpoint?
	return false;
};

#endif

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace Layers
{
	static constexpr uint8 NON_MOVING = 0;
	static constexpr uint8 MOVING = 1;
	static constexpr uint8 NUM_LAYERS = 2;
};

// Function that determines if two object layers can collide
static bool MyObjectCanCollide( JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2 )
{
	switch ( inObject1 )
	{
	case Layers::NON_MOVING:
		return inObject2 == Layers::MOVING; // Non moving only collides with moving
	case Layers::MOVING:
		return true; // Moving collides with everything
	default:
		assert( false );
		return false;
	}
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer NON_MOVING( 0 );
	static constexpr JPH::BroadPhaseLayer MOVING( 1 );
};

// Function that determines if two broadphase layers can collide
bool MyBroadPhaseCanCollide( JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2 )
{
	switch ( inLayer1 )
	{
	case Layers::NON_MOVING:
		return inLayer2 == BroadPhaseLayers::MOVING;
	case Layers::MOVING:
		return true;
	default:
		assert( false );
		return false;
	}
}

// An example contact listener
class MyContactListener : public JPH::ContactListener
{
public:
	// See: ContactListener
	JPH::ValidateResult OnContactValidate( const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::CollideShapeResult &inCollisionResult ) override
	{
		Com_DPrint( "Contact validate callback\n" );

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	void OnContactAdded( const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings ) override
	{
		Com_DPrint( "A contact was added\n" );
	}

	void OnContactPersisted( const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings ) override
	{
		Com_DPrint( "A contact was persisted\n" );
	}

	void OnContactRemoved( const JPH::SubShapeIDPair &inSubShapePair ) override
	{
		Com_DPrint( "A contact was removed\n" );
	}
};

// An example activation listener
class MyBodyActivationListener : public JPH::BodyActivationListener
{
public:
	void OnBodyActivated( const JPH::BodyID &inBodyID, void *inBodyUserData ) override
	{
		Com_DPrint( "A body got activated\n" );
	}

	void OnBodyDeactivated( const JPH::BodyID &inBodyID, void *inBodyUserData ) override
	{
		Com_DPrint( "A body went to sleep\n" );
	}
};

//=================================================================================================

static struct physicsLocal_t
{
	// Persistent

	JPH::TempAllocator *	tempAllocator;			// Allocator for temporary allocations
	JPH::JobSystem *		jobSystem;				// The job system that runs physics jobs
	JPH::PhysicsSystem *	physicsSystem;			// The physics system that simulates the world

	JPH::ObjectToBroadPhaseLayer objectToBroadphase;
	MyBodyActivationListener bodyActivationListener;
	MyContactListener contactListener;

	std::vector<JPH::Shape *> shapeCache;

	// Things

} vars;

//=================================================================================================

#define QUAKE2JOLT_FACTOR		0.0254				// Factor to convert game units to meters
#define JOLT2QUAKE_FACTOR		39.3700787402		// Factor to convert meters to game units

template< std::floating_point T >
inline constexpr T QUAKE2JPH( T x ) { return x * static_cast<T>( QUAKE2JOLT_FACTOR ); }

template< std::floating_point T >
inline constexpr T JPH2QUAKE( T x ) { return x * static_cast<T>( JOLT2QUAKE_FACTOR ); }

// Converts a Quake vec3_t to a Jolt SIMD Vec3
static JPH::Vec3 QuakeVec3ToJPHVec3( const vec3_t vec )
{
	return JPH::Vec3( vec[0], vec[1], vec[2] );
}

// Converts Quake Euler angles to a Jolt Quaternion
static JPH::Quat QuakeVec3ToJPHQuat( const vec3_t vec )
{
	return JPH::Quat::sEulerAngles( JPH::Vec3( vec[0], vec[2], vec[1] ) );
}

static JPH::EMotionType QuakeMotionTypeToJPHMotionType( motionType_t type )
{
	return static_cast<JPH::EMotionType>( type ); // Types are equivalent
}

static JPH::BodyInterface &GetBodyInterfaceNoLock()
{
	return vars.physicsSystem->GetBodyInterface();
}

//=================================================================================================

/*
=================================================
	Internals
=================================================
*/

void Init()
{
	// Install callbacks
	JPH::Trace = MyTrace;
	JPH_IF_ENABLE_ASSERTS( JPH::AssertFailed = MyAssertFailed; )

	// Register all Jolt physics types
	JPH::RegisterTypes();

	// We need a temp allocator for temporary allocations during the physics update. We're
	// pre-allocating 10 MB to avoid having to do allocations during the physics update. 
	// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
	// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
	// malloc / free.
	vars.tempAllocator = new JPH::TempAllocatorImpl( 16 * 1024 * 1024 );

	// We need a job system that will execute physics jobs on multiple threads. Typically
	// you would implement the JobSystem interface yourself and let Jolt Physics run on top
	// of your own job scheduler. JobSystemThreadPool is an example implementation.
	vars.jobSystem = new JPH::JobSystemThreadPool( JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, NUM_THREADS - 1 );

	// Create mapping table from object layer to broadphase layer
	vars.objectToBroadphase.resize( Layers::NUM_LAYERS );
	vars.objectToBroadphase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
	vars.objectToBroadphase[Layers::MOVING] = BroadPhaseLayers::MOVING;

	// Now we can create the actual physics system.
	vars.physicsSystem = new JPH::PhysicsSystem;
	vars.physicsSystem->Init( MAX_BODIES, NUM_BODY_MUTEXES, MAX_BODY_PAIRS, MAX_CONTACT_CONSTRAINTS, vars.objectToBroadphase, MyBroadPhaseCanCollide, MyObjectCanCollide );

	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	vars.physicsSystem->SetBodyActivationListener( &vars.bodyActivationListener );

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	vars.physicsSystem->SetContactListener( &vars.contactListener );

	JPH::Vec3 gravity = vars.physicsSystem->GetGravity();
	gravity = gravity.Swizzle<JPH::SWIZZLE_X, JPH::SWIZZLE_Z, JPH::SWIZZLE_Y>();
	//gravity *= JOLT2QUAKE_FACTOR;
	vars.physicsSystem->SetGravity( gravity );
}

void Shutdown()
{
	delete vars.physicsSystem;
	delete vars.jobSystem;
	delete vars.tempAllocator;
}

void Simulate( float deltaTime )
{
	// We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
	constexpr float cDeltaTime = 1.0f / 60.0f;

	constexpr uint numSteps = 16;
	for ( uint s = 0; s < numSteps; ++s )
	{
		vars.physicsSystem->Update( cDeltaTime, 1, 1, vars.tempAllocator, vars.jobSystem );
	}
}

/*
=================================================
	Shape Creation
=================================================
*/

shapeHandle_t CreateBoxShape( vec3_t halfExtent )
{
	JPH::Vec3 jphHalfExtent( halfExtent[0], halfExtent[1], halfExtent[2] );

	JPH::BoxShape *shape = new JPH::BoxShape( jphHalfExtent );
	shape->AddRef();

	vars.shapeCache.push_back( shape );

	return reinterpret_cast<shapeHandle_t>( shape );
}

shapeHandle_t CreateSphereShape( float radius )
{
	JPH::SphereShape *shape = new JPH::SphereShape( radius );
	shape->AddRef();

	vars.shapeCache.push_back( shape );

	return reinterpret_cast<shapeHandle_t>( shape );
}

void DestroyShape( shapeHandle_t handle )
{
	JPH::Shape *shape = reinterpret_cast<JPH::Shape *>( handle );
	
	shape->Release();
}

/*
=================================================
	Bodies
=================================================
*/

bodyID_t CreateAndAddBody( const bodyCreationSettings_t &settings, shapeHandle_t shape )
{
	JPH::BodyCreationSettings creationSettings(
		reinterpret_cast<JPH::Shape *>( shape ),
		QuakeVec3ToJPHVec3( settings.position ),
		QuakeVec3ToJPHQuat( settings.rotation ),
		QuakeMotionTypeToJPHMotionType( settings.motionType ),
		Layers::MOVING ); // TODO: Must be variable based on motion type...

	creationSettings.mFriction = settings.friction;
	creationSettings.mRestitution = settings.restitution;
	creationSettings.mLinearDamping = settings.linearDamping;
	creationSettings.mAngularDamping = settings.angularDamping;
	creationSettings.mMaxLinearVelocity = settings.maxLinearVelocity;
	creationSettings.mMaxAngularVelocity = settings.maxAngularVelocity;
	creationSettings.mGravityFactor = settings.gravityFactor;

	creationSettings.mInertiaMultiplier = settings.inertiaMultiplier;

	creationSettings.mMotionQuality = JPH::EMotionQuality::LinearCast;

	JPH::BodyInterface &bodyInterface = GetBodyInterfaceNoLock();

	JPH::BodyID bodyID = bodyInterface.CreateAndAddBody( creationSettings, JPH::EActivation::Activate ); // TODO: Must be variable based on motion type...

	return std::bit_cast<bodyID_t>( bodyID );
}

void RemoveAndDestroyBody( bodyID_t bodyID )
{
	JPH::BodyInterface &bodyInterface = GetBodyInterfaceNoLock();

	bodyInterface.RemoveBody( std::bit_cast<JPH::BodyID>( bodyID ) );
	bodyInterface.DestroyBody( std::bit_cast<JPH::BodyID>( bodyID ) );
}

void CreatePhysExplosion()
{
	JPH::BodyInterface &bodyInterface = GetBodyInterfaceNoLock();
}

void GetBodyPositionAndRotation( bodyID_t bodyID, vec3_t position, vec3_t angles )
{
	JPH::BodyInterface &bodyInterface = GetBodyInterfaceNoLock();

	JPH::Vec3 jphPos;
	JPH::Quat jphRot;

	bodyInterface.GetPositionAndRotation( std::bit_cast<JPH::BodyID>( bodyID ), jphPos, jphRot );

	jphPos.StoreFloat3( reinterpret_cast<JPH::Float3 *>( position ) );
	jphPos = jphRot.GetEulerAngles();

	// Z X Y
	JPH::Vec3 newPos( RAD2DEG( jphPos.GetY() ), RAD2DEG( jphPos.GetZ() ), RAD2DEG( jphPos.GetX() ) );

	newPos.StoreFloat3( reinterpret_cast<JPH::Float3 *>( angles ) );

	//angles[0] = 0.0f;
	//angles[1] = 0.0f;
	//angles[2] = 0.0f;
}

void SetLinearAndAngularVelocity( bodyID_t bodyID, vec3_t velocity, vec3_t avelocity )
{
	JPH::Vec3 jphLinearVelocity = QuakeVec3ToJPHVec3( velocity );
	JPH::Vec3 jphAngularVelocity = QuakeVec3ToJPHVec3( avelocity );

	JPH::BodyInterface &bodyInterface = GetBodyInterfaceNoLock();

	bodyInterface.SetLinearAndAngularVelocity( std::bit_cast<JPH::BodyID>( bodyID ), jphLinearVelocity, jphAngularVelocity );
}

JPH::PhysicsSystem *GetPhysicsSystem()
{
	return vars.physicsSystem;
}

void AddWorld( const JPH::VertexList &vertexList, const JPH::IndexedTriangleList &indexList )
{
	// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
	JPH::BodyInterface &bodyInterface = GetBodyInterfaceNoLock();

	// Next we can create a rigid body to serve as the floor, we make a large box
	// Create the settings for the collision volume (the shape). 
	// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
	JPH::MeshShapeSettings meshShapeSettings( vertexList, indexList );

	// Create the shape
	JPH::ShapeSettings::ShapeResult meshShapeResult = meshShapeSettings.Create();
	JPH::ShapeRefC meshShape = meshShapeResult.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

	// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
	JPH::BodyCreationSettings meshCreationSettings( meshShape, JPH::Vec3::sZero(), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);

	// Create the actual rigid body
	JPH::BodyID bodyID = bodyInterface.CreateAndAddBody( meshCreationSettings, JPH::EActivation::DontActivate ); // Note that if we run out of bodies this can return nullptr

	//AddDebugBall();
}

void LineTrace( const vec3 &start, const vec3 &end )
{
}

} // namespace PhysicsSystem

class CPhysicsSystem final : public IPhysicsSystem
{
public:
	void Simulate( float deltaTime ) override
	{
		Physics::Simulate( deltaTime );
	}

	shapeHandle_t CreateBoxShape( vec3_t halfExtent ) override
	{
		return Physics::CreateBoxShape( halfExtent );
	}

	shapeHandle_t CreateSphereShape( float radius ) override
	{
		return Physics::CreateSphereShape( radius );
	}

	void DestroyShape( shapeHandle_t handle ) override
	{
		Physics::DestroyShape( handle );
	}

	bodyID_t CreateAndAddBody( const bodyCreationSettings_t &settings, shapeHandle_t shape ) override
	{
		return Physics::CreateAndAddBody( settings, shape );
	}

	void RemoveAndDestroyBody( bodyID_t bodyID )
	{
		Physics::RemoveAndDestroyBody( bodyID );
	}

	void GetBodyPositionAndRotation( bodyID_t bodyID, vec3_t position, vec3_t angles ) override
	{
		Physics::GetBodyPositionAndRotation( bodyID, position, angles );
	}

	void SetLinearAndAngularVelocity( bodyID_t bodyID, vec3_t velocity, vec3_t avelocity ) override
	{
		Physics::SetLinearAndAngularVelocity( bodyID, velocity, avelocity );
	}

};

static CPhysicsSystem g_physSystemLocal;
IPhysicsSystem *g_physSystem = &g_physSystemLocal;

#endif
