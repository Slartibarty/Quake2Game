
#include "phys_local.h"

#include "phys_layers.h"
#include "phys_system.h"
#include "phys_body.h"

#include "phys_scene.h"

namespace PhysicsPrivate {

//-------------------------------------------------------------------------------------------------
// Generic
//-------------------------------------------------------------------------------------------------

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
		Assert( false );
		return false;
	}
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
		Assert( inLayer < Layers::NUM_LAYERS );
		return mObjectToBroadPhase[inLayer];
	}

#if defined( JPH_EXTERNAL_PROFILE ) || defined( JPH_PROFILE_ENABLED )
	const char *GetBroadPhaseLayerName( JPH::BroadPhaseLayer inLayer ) const override
	{
		switch ( (JPH::BroadPhaseLayer::Type)inLayer )
		{
		case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:														Assert( false ); return "INVALID";
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
		Assert( false );
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
		Com_Print( "A contact was added\n" );
#if 0
		Com_DPrint( "A contact was added\n" );

		cplane_t plane;
		csurface_t surf;
		memset( &plane, 0, sizeof( plane ) );
		memset( &surf, 0, sizeof( surf ) );

		JoltPositionToQuake( inManifold.mWorldSpaceNormal, plane.normal );

		edict_t *ent1 = (edict_t *)inBody1.GetUserData();
		edict_t *ent2 = (edict_t *)inBody2.GetUserData();

		ge->Phys_Impact( ent1, ent2, &plane, &surf );
#endif
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

static MyBPLayerInterfaceImpl s_bpLayerInterface;
static MyContactListener s_contactListener;

//-------------------------------------------------------------------------------------------------
// CPhysicsScene
//-------------------------------------------------------------------------------------------------

CPhysicsScene::CPhysicsScene()
{
	m_physicsSystem.Init(
		cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
		s_bpLayerInterface, MyBroadPhaseCanCollide, MyObjectCanCollide );

	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	//m_PhysicsSystem.SetBodyActivationListener( &vars.bodyActivationListener );

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	m_physicsSystem.SetContactListener( &s_contactListener );

	JPH::Vec3 gravity = m_physicsSystem.GetGravity();
	gravity = gravity.Swizzle<JPH::SWIZZLE_X, JPH::SWIZZLE_Z, JPH::SWIZZLE_Y>();
	m_physicsSystem.SetGravity( gravity );
}

CPhysicsScene::~CPhysicsScene()
{
}

//-------------------------------------------------------------------------------------------------

void CPhysicsScene::Simulate( float deltaTime )
{
	// We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
	constexpr float cDeltaTime = 1.0f / 60.0f;

	CPhysicsSystem *pSys = CPhysicsSystem::GetInstance();

	constexpr uint numSteps = 8;
	for ( uint step = 0; step < numSteps; ++step )
	{
		m_physicsSystem.Update( cDeltaTime, 1, 1, pSys->GetTempAllocator(), pSys->GetJobSystem() );
	}
}

//-------------------------------------------------------------------------------------------------

IPhysicsBody *CPhysicsScene::CreateAndAddBody( const bodyCreationSettings_t &settings, IPhysicsShape *pShape, void *pUserData )
{
	const bool isStatic = settings.motionType == MOTION_STATIC;
	const uint8 layer = isStatic ? Layers::NON_MOVING : Layers::MOVING;
	const JPH::EActivation activationState = isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate;

	JPH::BodyCreationSettings creationSettings(
		reinterpret_cast<JPH::Shape *>( pShape ),
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

	JPH::BodyInterface &bodyInterface = m_physicsSystem.GetBodyInterfaceNoLock();

	JPH::Body *pBody = bodyInterface.CreateBody( creationSettings );

	pBody->SetUserData( reinterpret_cast<uint64>( pUserData ) );

	bodyInterface.AddBody( pBody->GetID(), activationState );

	return new CPhysicsBody( pBody, &m_physicsSystem );
}

void CPhysicsScene::CreateAndAddBody_World( void *secretVertexList, void *secretIndexList )
{
	JPH::VertexList &vertexList = *reinterpret_cast<JPH::VertexList *>( secretVertexList );
	JPH::IndexedTriangleList &indexList = *reinterpret_cast<JPH::IndexedTriangleList *>( secretIndexList );

	for ( JPH::Float3 &vert : vertexList )
	{
		vert.x = QuakeToJolt( vert.x );
		vert.y = QuakeToJolt( vert.y );
		vert.z = QuakeToJolt( vert.z );
	}

	// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
	JPH::BodyInterface &bodyInterface = m_physicsSystem.GetBodyInterfaceNoLock();

	// Next we can create a rigid body to serve as the floor, we make a large box
	// Create the settings for the collision volume (the shape). 
	// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
	JPH::MeshShapeSettings meshShapeSettings( vertexList, indexList );

	// Create the shape
	JPH::ShapeSettings::ShapeResult meshShapeResult = meshShapeSettings.Create();
	JPH::ShapeRefC meshShape = meshShapeResult.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

	// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
	JPH::BodyCreationSettings meshCreationSettings( meshShape, JPH::Vec3::sZero(), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING );

	// Create the actual rigid body
	JPH::Body *body = bodyInterface.CreateBody( meshCreationSettings ); // Note that if we run out of bodies this can return nullptr

	bodyInterface.AddBody( body->GetID(), JPH::EActivation::DontActivate );

	m_pWorldBody = body;
}

void CPhysicsScene::SetWorldBodyUserData( void *pUserData )
{
	Assert( m_pWorldBody );
	m_pWorldBody->SetUserData( reinterpret_cast<uint64>( pUserData ) );
}

void CPhysicsScene::RemoveAndDestroyBody( IPhysicsBody *pBody )
{
	JPH::BodyInterface &bodyInterface = m_physicsSystem.GetBodyInterfaceNoLock();

	CPhysicsBody *pBodyInternal = static_cast<CPhysicsBody *>( pBody );

	bodyInterface.RemoveBody( pBodyInternal->GetBodyID() );
	bodyInterface.DestroyBody( pBodyInternal->GetBodyID() );

	delete pBodyInternal;
}

//-------------------------------------------------------------------------------------------------

void *CPhysicsScene::GetInternalStructure()
{
	return reinterpret_cast<void *>( &m_physicsSystem );
}

} // namespace PhysicsPrivate
