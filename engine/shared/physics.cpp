
#if 1

// TODO: Move server specific code to the server!
#include "../server/sv_local.h"

#include <Jolt.h>
#include <RegisterTypes.h>

#include <Core/TempAllocator.h>
#include <Core/JobSystemThreadPool.h>

#include <Physics/PhysicsSettings.h>
#include <Physics/PhysicsSystem.h>

#include <Physics/Collision/CastResult.h>
#include <Physics/Collision/RayCast.h>
#include <Physics/Collision/NarrowPhaseQuery.h>
#include <Physics/Collision/CollisionCollector.h>
#include <Physics/Collision/CollisionCollectorImpl.h>
#include <Physics/Collision/ShapeCast.h>
#include <Physics/Collision/CollisionDispatch.h>

#include <Physics/Collision/Shape/BoxShape.h>
#include <Physics/Collision/Shape/CapsuleShape.h>
#include <Physics/Collision/Shape/CylinderShape.h>
#include <Physics/Collision/Shape/MeshShape.h>
#include <Physics/Collision/Shape/SphereShape.h>
#include <Physics/Collision/Shape/TaperedCapsuleShape.h>
#include <Physics/Collision/Shape/RotatedTranslatedShape.h>

#include <Physics/Body/BodyCreationSettings.h>
#include <Physics/Body/BodyActivationListener.h>

#include "physics.h"

#define DIST_EPSILON 0.03125f

extern csurface_t s_nullsurface;

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

#define QUAKE2JOLT_FACTOR		0.0254f				// Factor to convert game units to meters
#define JOLT2QUAKE_FACTOR		39.3700787402f		// Factor to convert meters to game units

inline constexpr float QUAKE2JOLT( float x ) { return x * QUAKE2JOLT_FACTOR; }
inline constexpr float JOLT2QUAKE( float x ) { return x * JOLT2QUAKE_FACTOR; }

// Position

static JPH::Vec3 QuakePositionToJolt( const vec3_t pos )
{
	return JPH::Vec3( QUAKE2JOLT( pos[0] ), QUAKE2JOLT( pos[1] ), QUAKE2JOLT( pos[2] ) );
}

static void JoltPositionToQuake( JPH::Vec3Arg jpos, vec3_t qpos )
{
	jpos *= JOLT2QUAKE_FACTOR;
	jpos.StoreFloat3( (JPH::Float3 *)qpos );
}

// Rotation

// Converts Quake Euler angles to a Jolt Quaternion
static JPH::Quat QuakeEulerToJoltQuat( const vec3_t euler )
{
	vec4_t quat;
	AngleQuaternion( euler, quat );
	return JPH::Quat( quat[0], quat[1], quat[2], quat[3] );
}

static void JoltQuatToQuakeEuler( JPH::QuatArg quat, vec3_t euler )
{
	vec4_t quakeQuat{ quat.GetX(), quat.GetY(), quat.GetZ(), quat.GetW() };

	QuaternionAngles( quakeQuat, euler );

}

static JPH::EMotionType QuakeMotionTypeToJPHMotionType( motionType_t type )
{
	return static_cast<JPH::EMotionType>( type ); // Types are equivalent
}

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

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class MyBPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
	MyBPLayerInterfaceImpl()
	{
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	uint GetNumBroadPhaseLayers() const override
	{
		return Layers::NUM_LAYERS;
	}

	JPH::BroadPhaseLayer GetBroadPhaseLayer( JPH::ObjectLayer inLayer ) const override
	{
		assert( inLayer < Layers::NUM_LAYERS );
		return mObjectToBroadPhase[inLayer];
	}

#if defined( JPH_EXTERNAL_PROFILE ) || defined( JPH_PROFILE_ENABLED )
	const char *GetBroadPhaseLayerName( JPH::BroadPhaseLayer inLayer ) const override
	{
		switch ( (JPH::BroadPhaseLayer::Type)inLayer )
		{
		case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:														assert( false ); return "INVALID";
		}
	}
#endif

private:
	JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

// Function that determines if two broadphase layers can collide
static bool MyBroadPhaseCanCollide( JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2 )
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
#if 1
class MyContactListener final : public JPH::ContactListener
{
public:
	// See: ContactListener
#if 0
	JPH::ValidateResult OnContactValidate( const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::CollideShapeResult &inCollisionResult ) override
	{
		Com_DPrint( "Contact validate callback\n" );

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}
#endif

	void OnContactAdded( const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings ) override
	{
		Com_DPrint( "A contact was added\n" );

		cplane_t plane;
		csurface_t surf;
		memset( &plane, 0, sizeof( plane ) );
		memset( &surf, 0, sizeof( surf ) );

		JoltPositionToQuake( inManifold.mWorldSpaceNormal, plane.normal );

		edict_t *ent1 = (edict_t *)inBody1.GetUserData();
		edict_t *ent2 = (edict_t *)inBody2.GetUserData();

		ge->Phys_Impact( ent1, ent2, &plane, &surf );
	}

#if 0
	void OnContactPersisted( const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings ) override
	{
		Com_DPrint( "A contact was persisted\n" );
	}

	void OnContactRemoved( const JPH::SubShapeIDPair &inSubShapePair ) override
	{
		Com_DPrint( "A contact was removed\n" );
	}
#endif
};
#endif

// An example activation listener
#if 0
class MyBodyActivationListener final : public JPH::BodyActivationListener
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
#endif

//=================================================================================================

static struct physicsLocal_t
{
	// Persistent

	JPH::TempAllocator *	tempAllocator;			// Allocator for temporary allocations
	JPH::JobSystem *		jobSystem;				// The job system that runs physics jobs
	JPH::PhysicsSystem *	physicsSystem;			// The physics system that simulates the world
	JPH::Body *				worldBody;				// The world body!

	MyBPLayerInterfaceImpl bpLayerInterface;
	//MyBodyActivationListener bodyActivationListener;
	MyContactListener contactListener;

	std::vector<JPH::Shape *> shapeCache;

	// Things

} vars;

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

	// Now we can create the actual physics system.
	vars.physicsSystem = new JPH::PhysicsSystem;
	vars.physicsSystem->Init( MAX_BODIES, NUM_BODY_MUTEXES, MAX_BODY_PAIRS, MAX_CONTACT_CONSTRAINTS, vars.bpLayerInterface, MyBroadPhaseCanCollide, MyObjectCanCollide );

	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	//vars.physicsSystem->SetBodyActivationListener( &vars.bodyActivationListener );

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	vars.physicsSystem->SetContactListener( &vars.contactListener );

	JPH::Vec3 gravity = vars.physicsSystem->GetGravity();
	gravity = gravity.Swizzle<JPH::SWIZZLE_X, JPH::SWIZZLE_Z, JPH::SWIZZLE_Y>();
	vars.physicsSystem->SetGravity( gravity );
}

void Shutdown()
{
	delete vars.physicsSystem;
	delete vars.jobSystem;
	delete vars.tempAllocator;
}

void SetContactListener( void *listener )
{
}

void Simulate( float deltaTime )
{
	// We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
	constexpr float cDeltaTime = 1.0f / 60.0f;

	constexpr uint numSteps = 8;
	for ( uint step = 0; step < numSteps; ++step )
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
	JPH::BoxShape *shape = new JPH::BoxShape( QuakePositionToJolt( halfExtent ) );
	shape->AddRef();

	//vars.shapeCache.push_back( shape );

	return reinterpret_cast<shapeHandle_t>( shape );
}

shapeHandle_t CreateCapsuleShape( float halfHeightOfCylinder, float radius )
{
	JPH::CapsuleShape *shape = new JPH::CapsuleShape( QUAKE2JOLT( halfHeightOfCylinder ), QUAKE2JOLT( radius ) );

	JPH::RotatedTranslatedShapeSettings settings( JPH::Vec3( 0.0f, 0.0f, 22.5f ), JPH::Quat::sIdentity(), shape );

	JPH::ShapeSettings::ShapeResult shapeResult;
	JPH::RotatedTranslatedShape *shape2 = new JPH::RotatedTranslatedShape( settings, shapeResult );
	shape->AddRef();

	//vars.shapeCache.push_back( shape );

	return reinterpret_cast<shapeHandle_t>( shape2 );
}

shapeHandle_t CreateCylinderShape( float halfHeight, float radius )
{
	JPH::CylinderShape *shape = new JPH::CylinderShape( QUAKE2JOLT( halfHeight ), QUAKE2JOLT( radius ) );
	shape->AddRef();

	//vars.shapeCache.push_back( shape );

	return reinterpret_cast<shapeHandle_t>( shape );
}

shapeHandle_t CreateSphereShape( float radius )
{
	JPH::SphereShape *shape = new JPH::SphereShape( QUAKE2JOLT( radius ) );
	shape->AddRef();

	//vars.shapeCache.push_back( shape );

	return reinterpret_cast<shapeHandle_t>( shape );
}

shapeHandle_t CreateTaperedCapsuleShape( float halfHeightOfTaperedCylinder, float topRadius, float bottomRadius )
{
	// Tapered capsules are a little more complex, so they have error handling routines

	JPH::TaperedCapsuleShapeSettings settings( QUAKE2JOLT( halfHeightOfTaperedCylinder ), QUAKE2JOLT( topRadius ), QUAKE2JOLT( bottomRadius ) );

	JPH::ShapeSettings::ShapeResult shapeResult;
	JPH::TaperedCapsuleShape *shape = new JPH::TaperedCapsuleShape( settings, shapeResult );
	shape->AddRef();

	assert( !shapeResult.HasError() );

	//vars.shapeCache.push_back( shape );

	return reinterpret_cast<shapeHandle_t>( shape );
}

void DestroyShape( shapeHandle_t handle )
{
	JPH::Shape *shape = reinterpret_cast<JPH::Shape *>( handle );

	assert( shape->GetRefCount() == 1 );
	
	shape->Release();
}

const shapeHandle_t GetShape( bodyID_t bodyID )
{
	const JPH::BodyLockInterfaceLocking &lockInterface = vars.physicsSystem->GetBodyLockInterface();

	JPH::BodyLockRead lock( lockInterface, static_cast<JPH::BodyID>( bodyID ) );
	if ( lock.Succeeded() )
	{
		// ffs this shouldn't be a bit cast but reinterpret_cast doesn't let me do the conversion
		return std::bit_cast<const shapeHandle_t>( lock.GetBody().GetShape() );
	}
	else
	{
		Com_FatalError( "BodyLockRead failed!\n" );
	}
}

/*
=================================================
	Bodies
=================================================
*/

bodyID_t CreateAndAddBody( const bodyCreationSettings_t &settings, const shapeHandle_t shape, void *userData )
{
	const bool isStatic = settings.motionType == MOTION_STATIC;
	const uint8 layer = isStatic ? Layers::NON_MOVING : Layers::MOVING;
	const JPH::EActivation activationState = isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate;

	JPH::BodyCreationSettings creationSettings(
		reinterpret_cast<JPH::Shape *>( shape ),
		QuakePositionToJolt( settings.position ),
		QuakeEulerToJoltQuat( settings.rotation ),
		QuakeMotionTypeToJPHMotionType( settings.motionType ),
		layer );

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

	JPH::Body *body = bodyInterface.CreateBody( creationSettings );

	body->SetUserData( (uint64)userData );

	bodyInterface.AddBody( body->GetID(), activationState );

	return std::bit_cast<bodyID_t>( body->GetID() );
}

void RemoveAndDestroyBody( bodyID_t bodyID )
{
	JPH::BodyInterface &bodyInterface = GetBodyInterfaceNoLock();

	bodyInterface.RemoveBody( std::bit_cast<JPH::BodyID>( bodyID ) );
	bodyInterface.DestroyBody( std::bit_cast<JPH::BodyID>( bodyID ) );
}

void GetBodyPositionAndRotation( bodyID_t bodyID, vec3_t position, vec3_t rotation )
{
	JPH::BodyInterface &bodyInterface = GetBodyInterfaceNoLock();

	JPH::Vec3 jphPos;
	JPH::Quat jphRot;

	JPH::BodyID jphBodyID = std::bit_cast<JPH::BodyID>( bodyID );
	bodyInterface.GetPositionAndRotation( jphBodyID, jphPos, jphRot );

	JoltPositionToQuake( jphPos, position );
	JoltQuatToQuakeEuler( jphRot, rotation );
}

void AddLinearVelocity( bodyID_t bodyID, vec3_t velocity )
{
	JPH::Vec3 jphLinearVelocity = QuakePositionToJolt( velocity );

	JPH::BodyInterface &bodyInterface = GetBodyInterfaceNoLock();

	JPH::BodyID jphBodyID = std::bit_cast<JPH::BodyID>( bodyID );
	bodyInterface.AddLinearVelocity( jphBodyID, jphLinearVelocity );
}

void SetLinearAndAngularVelocity( bodyID_t bodyID, vec3_t velocity, vec3_t avelocity )
{
	JPH::Vec3 jphLinearVelocity = QuakePositionToJolt( velocity );
	JPH::Vec3 jphAngularVelocity = QuakePositionToJolt( avelocity );

	JPH::BodyInterface &bodyInterface = GetBodyInterfaceNoLock();

	JPH::BodyID jphBodyID = std::bit_cast<JPH::BodyID>( bodyID );
	bodyInterface.SetLinearAndAngularVelocity( jphBodyID, jphLinearVelocity, jphAngularVelocity );
}

void GetLinearAndAngularVelocity( bodyID_t bodyID, vec3_t velocity, vec3_t avelocity )
{
	JPH::BodyInterface &bodyInterface = GetBodyInterfaceNoLock();

	JPH::Vec3 jphLinearVelocity, jphAngularVelocity;

	JPH::BodyID jphBodyID = std::bit_cast<JPH::BodyID>( bodyID );
	bodyInterface.GetLinearAndAngularVelocity( jphBodyID, jphLinearVelocity, jphAngularVelocity );

	jphLinearVelocity *= JOLT2QUAKE_FACTOR;
	jphAngularVelocity *= JOLT2QUAKE_FACTOR;

	jphLinearVelocity.StoreFloat3( (JPH::Float3 *)velocity );
	jphAngularVelocity.StoreFloat3( (JPH::Float3 *)avelocity );
}

JPH::PhysicsSystem *GetPhysicsSystem()
{
	return vars.physicsSystem;
}

void AddWorld( JPH::VertexList &vertexList, JPH::IndexedTriangleList &indexList )
{
	for ( JPH::Float3 &vert : vertexList )
	{
		vert.x = QUAKE2JOLT( vert.x );
		vert.y = QUAKE2JOLT( vert.y );
		vert.z = QUAKE2JOLT( vert.z );
	}

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
	JPH::Body *body = bodyInterface.CreateBody( meshCreationSettings ); // Note that if we run out of bodies this can return nullptr

	bodyInterface.AddBody( body->GetID(), JPH::EActivation::DontActivate );

	vars.worldBody = body;

	//AddDebugBall();
}

void RegisterWorld( edict_t *ent )
{
	vars.worldBody->SetUserData( (uint64)ent );
}

void Trace( const RayCast &rayCast, const shapeHandle_t shapeHandle, const vec3_t shapeOrigin, const vec3_t shapeAngles, trace_t &trace )
{
	memset( &trace, 0, sizeof( trace ) );
	trace.surface = &s_nullsurface;
	trace.fraction = 1.0f;
	trace.contents = CONTENTS_SOLID;

	JPH::Vec3 origin = QuakePositionToJolt( rayCast.start );
	JPH::Vec3 direction = QuakePositionToJolt( rayCast.direction );
	JPH::Vec3 halfExtent = QuakePositionToJolt( rayCast.halfExtent );

	bool isRay = halfExtent.ReduceMin() < JPH::cDefaultConvexRadius;
	bool isCollide = direction.IsNearZero();

	JPH::Shape *shape = reinterpret_cast<JPH::Shape *>( shapeHandle );

	JPH::Vec3 position = QuakePositionToJolt( shapeOrigin );
	JPH::Quat rotation = QuakeEulerToJoltQuat( shapeAngles );

	JPH::Mat44 queryTransform = JPH::Mat44::sRotationTranslation( rotation, position + rotation * shape->GetCenterOfMass() );

	if ( isRay )
	{
		// Create ray
		JPH::RayCast baseRay = { origin, direction };
		// Transform our ray by the inverse of the collideOrigin/collideAngles
		JPH::RayCast joltRay = baseRay.Transformed( queryTransform.InversedRotationTranslation() );

		JPH::RayCastResult hit;
		bool bHit = shape->CastRay( joltRay, JPH::SubShapeIDCreator(), hit );

		trace.fraction = bHit ? hit.mFraction : 1.0f;
		vec3_t distance;
		VectorScale( rayCast.direction, trace.fraction, distance );
		VectorAdd( rayCast.start, distance, trace.endpos );
		trace.allsolid = trace.fraction == 0.0f;
		trace.startsolid = trace.fraction == 0.0f;
		if ( bHit )
		{
			// Set up the trace plane
			JPH::Vec3 normal = queryTransform.GetRotation() * shape->GetSurfaceNormal( hit.mSubShapeID2, joltRay.GetPointOnRay( hit.mFraction ) );

			VectorSet( trace.plane.normal, normal.GetX(), normal.GetY(), normal.GetZ() );
			trace.plane.dist = DotProduct( trace.endpos, trace.plane.normal );
		}
	}
	else
	{
		constexpr float testEpsilon = QUAKE2JOLT( DIST_EPSILON );

		JPH::BoxShape boxShape( halfExtent );
		JPH::ShapeCast initialShapeCast( &boxShape, JPH::Vec3::sReplicate( 1.0f ), JPH::Mat44::sTranslation( origin ), direction );
		JPH::ShapeCast shapeCast = initialShapeCast.PostTransformed( queryTransform.InversedRotationTranslation() );

		JPH::ShapeCastSettings settings;
		settings.mBackFaceModeTriangles = JPH::EBackFaceMode::IgnoreBackFaces;
		settings.mBackFaceModeConvex = JPH::EBackFaceMode::CollideWithBackFaces;
		settings.mCollisionTolerance = testEpsilon;
		settings.mPenetrationTolerance = testEpsilon;
		settings.mUseShrunkenShapeAndConvexRadius = true;

		JPH::ClosestHitCollisionCollector< JPH::CastShapeCollector > collector;
		JPH::CollisionDispatch::sCastShapeVsShape( shapeCast, settings, shape, JPH::Vec3::sReplicate( 1.0f ), JPH::ShapeFilter(), JPH::Mat44::sIdentity(), JPH::SubShapeIDCreator(), JPH::SubShapeIDCreator(), collector);

		float flInitialFraction = collector.HadHit() ? collector.GetEarlyOutFraction() : 1.0f;
		if ( collector.HadHit() )
		{
			JPH::Vec3 normal = queryTransform.GetRotation() * shape->GetSurfaceNormal( collector.mHit.mSubShapeID2, collector.mHit.mContactPointOn2 );
			VectorSet( trace.plane.normal, normal.GetX(), normal.GetY(), normal.GetZ() );

			vec3_t rayDir;
			VectorCopy( rayCast.direction, rayDir );
			const float flRayLength = VectorLength( rayDir );

			float ooBaseLength = 0.0f;
			if ( flRayLength )
			{
				VectorNormalize( rayDir );
				ooBaseLength = 1.0f / flRayLength;
			}
			float flHitLength = flRayLength * flInitialFraction;
			float flDot = DotProduct( rayDir, trace.plane.normal );
			if ( flDot < 0.0f )
				flHitLength += DIST_EPSILON / flDot;

			flHitLength = Max( flHitLength, 0.0f );

			trace.fraction = flHitLength * ooBaseLength;
		}
		else
		{
			trace.fraction = 1.0f;
		}

		vec3_t distance;
		VectorScale( rayCast.direction, trace.fraction, distance );
		VectorAdd( rayCast.start, distance, trace.endpos );

		trace.plane.dist = DotProduct( trace.endpos, trace.plane.normal );

		trace.allsolid = flInitialFraction == 0.0f;
		trace.startsolid = flInitialFraction == 0.0f;
	}
}

void ClientPlayerTrace( const RayCast &rayCast, const vec3_t shapeOrigin, const vec3_t shapeAngles, trace_t &trace )
{
	// Hardcoded player halfextents
	JPH::BoxShape shape( JPH::Vec3( 10.0f, 10.0f, 10.0f ) );
	Trace( rayCast, reinterpret_cast<shapeHandle_t>( &shape ), shapeOrigin, shapeAngles, trace );
}

} // namespace PhysicsSystem

class CPhysicsSystem final : public IPhysicsSystem
{
public:
	void RegisterWorld( edict_t *ent ) override
	{
		Physics::RegisterWorld( ent );
	}

	void Simulate( float deltaTime ) override
	{
		Physics::Simulate( deltaTime );
	}

	shapeHandle_t CreateBoxShape( vec3_t halfExtent ) override
	{
		return Physics::CreateBoxShape( halfExtent );
	}

	shapeHandle_t CreateCapsuleShape( float halfHeightOfCylinder, float radius ) override
	{
		return Physics::CreateCapsuleShape( halfHeightOfCylinder, radius );
	}

	shapeHandle_t CreateCylinderShape( float halfHeight, float radius )
	{
		return Physics::CreateCylinderShape( halfHeight, radius );
	}

	shapeHandle_t CreateSphereShape( float radius ) override
	{
		return Physics::CreateSphereShape( radius );
	}

	shapeHandle_t CreateTaperedCapsuleShape( float halfHeightOfTaperedCylinder, float topRadius, float bottomRadius ) override
	{
		return Physics::CreateTaperedCapsuleShape( halfHeightOfTaperedCylinder, topRadius, bottomRadius );
	}

	void DestroyShape( shapeHandle_t handle ) override
	{
		Physics::DestroyShape( handle );
	}

	bodyID_t CreateAndAddBody( const bodyCreationSettings_t &settings, shapeHandle_t shape, void *userData ) override
	{
		return Physics::CreateAndAddBody( settings, shape, userData );
	}

	void RemoveAndDestroyBody( bodyID_t bodyID ) override
	{
		Physics::RemoveAndDestroyBody( bodyID );
	}

	void GetBodyPositionAndRotation( bodyID_t bodyID, vec3_t position, vec3_t rotation ) override
	{
		Physics::GetBodyPositionAndRotation( bodyID, position, rotation );
	}

	void AddLinearVelocity( bodyID_t bodyID, vec3_t velocity ) override
	{
		Physics::AddLinearVelocity( bodyID, velocity );
	}

	void SetLinearAndAngularVelocity( bodyID_t bodyID, vec3_t velocity, vec3_t avelocity ) override
	{
		Physics::SetLinearAndAngularVelocity( bodyID, velocity, avelocity );
	}

	void GetLinearAndAngularVelocity( bodyID_t bodyID, vec3_t velocity, vec3_t avelocity ) override
	{
		Physics::GetLinearAndAngularVelocity( bodyID, velocity, avelocity );
	}

};

static CPhysicsSystem g_physSystemLocal;
IPhysicsSystem *g_physSystem = &g_physSystemLocal;

#endif
