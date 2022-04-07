//
// Physics engine interface
//

#pragma once

enum motionType_t : uint8
{
	MOTION_STATIC,
	MOTION_KINEMATIC,
	MOTION_DYNAMIC
};

struct bodyCreationSettings_t
{
	vec3_t position;
	vec3_t rotation;

	motionType_t motionType = MOTION_DYNAMIC;
	float friction = 0.2f;
	float restitution = 0.0f;
	float linearDamping = 0.05f;
	float angularDamping = 0.05f;
	float maxLinearVelocity = 500.0f;
	float maxAngularVelocity = 0.25f * M_PI_F * 60.0f;
	float gravityFactor = 1.0f;

	float inertiaMultiplier = 1.0f;
};

// Forward declarations
class IPhysicsScene;
class IPhysicsShape;
class IPhysicsBody;

//
// Accessor for stateless operations and the root of all evil
//
abstract_class IPhysicsSystem
{
public:
	virtual void Init() = 0;
	virtual void Shutdown() = 0;

	// Create / destroy a physics scene
	virtual IPhysicsScene *CreateScene() = 0;
	virtual void DestroyScene( IPhysicsScene *pScene ) = 0;

	// Create / destroy various shapes
	virtual IPhysicsShape *CreateBoxShape( vec3_t halfExtent ) = 0;
	virtual IPhysicsShape *CreateSphereShape( float radius ) = 0;
	//virtual IPhysicsShape *CreateMeshShape( JPH::VertexList &vertexList, JPH::IndexedTriangleList &indexList ) = 0;
	virtual void DestroyShape( IPhysicsShape *pShape ) = 0;

};

//
// Represents a JPH::PhysicsSystem
//
abstract_class IPhysicsScene
{
public:
	virtual void Simulate( float deltaTime ) = 0;

	virtual IPhysicsBody *CreateAndAddBody( const bodyCreationSettings_t &settings, IPhysicsShape *pShape, void *pUserData ) = 0;
	virtual void CreateAndAddBody_World( void *vertexList, void *indexList ) = 0; // HACK
	virtual void SetWorldBodyUserData( void *pUserData ) = 0; // HACK
	virtual void RemoveAndDestroyBody( IPhysicsBody *pBody ) = 0;

	// Returns the JPH::PhysicsSystem
	virtual void *GetInternalStructure() = 0;

};

//
// Represents a JPH::Shape
//
#if 0
abstract_class IPhysicsShape
{
public:

};
#endif

//
// Represents a JPH::Body
//
abstract_class IPhysicsBody
{
public:
	virtual void GetPositionAndRotation( vec3_t position, vec3_t rotation ) = 0;

	virtual void SetLinearAndAngularVelocity( const vec3_t velocity, const vec3_t avelocity ) = 0;
	virtual void GetLinearAndAngularVelocity( vec3_t velocity, vec3_t avelocity ) = 0;
	virtual void AddLinearVelocity( const vec3_t velocity ) = 0;

};

#ifdef Q_ENGINE
namespace Physics { IPhysicsSystem *GetPhysicsSystem(); }
#endif
