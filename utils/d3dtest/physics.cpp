
#include "engine.h"

#define JPH_STAT_COLLECTOR
#define JPH_PROFILE_ENABLED
#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED

#include <Jolt.h>
#include <RegisterTypes.h>
#include <Core/TempAllocator.h>
#include <Core/JobSystemThreadPool.h>
#include <Physics/PhysicsSettings.h>
#include <Physics/PhysicsSystem.h>
#include <Physics/Collision/Shape/BoxShape.h>
#include <Physics/Collision/Shape/SphereShape.h>
#include <Physics/Collision/Shape/MeshShape.h>
#include <Physics/Body/BodyCreationSettings.h>
#include <Physics/Body/BodyActivationListener.h>

#include "renderer/tinyobj_loader_c.h"

namespace PhysicsSystem
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

	Com_DPrintf( "[JPH] %s\n", msg );
};

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool MyAssertFailed( const char *inExpression, const char *inMessage, const char *inFile, uint inLine )
{
	Com_DPrintf( "[JPH] %s:%u: (%s) %s\n", inFile, inLine, inExpression, inMessage != nullptr ? inMessage : "" );

	// Breakpoint
	return true;
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

	// Things

} vars;

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

struct threeIndices_t
{
	tinyobj_vertex_index_t face1;
	tinyobj_vertex_index_t face2;
	tinyobj_vertex_index_t face3;
};

struct objContext_t
{
	char *buffer;
	size_t bufferLength;
};

static void LoadOBJ_Callback( void *ctx, const char *filename, int is_mtl, const char *obj_filename, char **buf, size_t *len )
{
	objContext_t *context = (objContext_t *)ctx;

	*buf = context->buffer;
	*len = context->bufferLength;
}

static void GetDataFromOBJ( void *buffer, fsSize_t bufferLength, JPH::VertexList &vertexList, JPH::IndexedTriangleList &indexList )
{
	tinyobj_attrib_t attrib;
	tinyobj_shape_t *shapes;
	size_t numShapes;
	tinyobj_material_t *materials;
	size_t numMaterials;

	objContext_t context{ (char *)buffer, (size_t)bufferLength };

	int result = tinyobj_parse_obj( &attrib, &shapes, &numShapes,
		&materials, &numMaterials, nullptr, LoadOBJ_Callback, (void *)&context, TINYOBJ_FLAG_TRIANGULATE );
	assert( result == TINYOBJ_SUCCESS );

	vertexList.resize( attrib.num_vertices );
	for ( uint i = 0; i < attrib.num_vertices; ++i )
	{
		vertexList[i] = ((JPH::Float3 *)attrib.vertices)[i];
	}

	indexList.resize( attrib.num_faces / 3 );
	for ( uint i = 0; i < attrib.num_faces / 3; ++i )
	{
		indexList[i].mIdx[0] = ( (threeIndices_t *)attrib.faces )[i].face1.v_idx;
		indexList[i].mIdx[1] = ( (threeIndices_t *)attrib.faces )[i].face2.v_idx;
		indexList[i].mIdx[2] = ( (threeIndices_t *)attrib.faces )[i].face3.v_idx;
	}
}

void AddBodyForOBJ( void *buffer, fsSize_t bufferLength )
{
	JPH::VertexList vertexList;
	JPH::IndexedTriangleList indexList;

	GetDataFromOBJ( buffer, bufferLength, vertexList, indexList );

	// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
	JPH::BodyInterface &bodyInterface = vars.physicsSystem->GetBodyInterfaceNoLock();

	// Next we can create a rigid body to serve as the floor, we make a large box
	// Create the settings for the collision volume (the shape). 
	// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
	JPH::MeshShapeSettings meshShapeSettings( vertexList, indexList );

	// Create the shape
	JPH::ShapeSettings::ShapeResult meshShapeResult = meshShapeSettings.Create();
	JPH::ShapeRefC meshShape = meshShapeResult.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

	// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
	JPH::BodyCreationSettings meshCreationSettings( meshShape, JPH::Vec3( 0.0f, -1.0f, 0.0f ), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING );

	// Create the actual rigid body
	JPH::Body *body = bodyInterface.CreateBody( meshCreationSettings ); // Note that if we run out of bodies this can return nullptr

	// Add it to the world
	bodyInterface.AddBody( body->GetID(), JPH::EActivation::DontActivate );
}

} // namespace PhysicsSystem
