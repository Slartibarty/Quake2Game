
#pragma once

#include <Jolt.h>

#include <Physics/PhysicsSystem.h>

#include <Geometry/IndexedTriangle.h>

#include "../../common/physics_interface.h"

namespace Physics
{
	// Convenience structure
	// should move this elsewhere
	struct RayCast
	{
		vec3_t start, direction;	// Origin and distance to travel for the cast
		vec3_t halfExtent;			// Half extents of the AABB to cast

		RayCast( const vec3_t inStart, const vec3_t inEnd, const vec3_t inMins, const vec3_t inMaxs )
		{
			VectorCopy( inStart, start );
			VectorSubtract( inEnd, inStart, direction );

			VectorSubtract( inMaxs, inMins, halfExtent );
			VectorScale( halfExtent, 0.5f, halfExtent );
		}
	};

	//
	// Internals
	//

	void Init();
	void Shutdown();
	void SetContactListener( void *listener );
	void Simulate( float deltaTime );

	// Clip a ray against a specific entity
	void Trace( const RayCast &rayCast, const shapeHandle_t shapeHandle, const vec3_t shapeOrigin, const vec3_t shapeAngles, trace_t &inTrace );
	void ClientPlayerTrace( const RayCast &rayCast, const vec3_t shapeOrigin, const vec3_t shapeAngles, trace_t &trace );

	//
	// Shape Creation
	//

	shapeHandle_t CreateBoxShape( vec3_t halfExtent );
	shapeHandle_t CreateCapsuleShape( float halfHeightOfCylinder, float radius );
	shapeHandle_t CreateCylinderShape( float halfHeight, float radius );
	shapeHandle_t CreateSphereShape( float radius );
	shapeHandle_t CreateTaperedCapsuleShape( float halfHeightOfTaperedCylinder, float topRadius, float bottomRadius );
	void DestroyShape( shapeHandle_t handle );

	const shapeHandle_t GetShape( bodyID_t bodyID );

	//
	// Bodies
	//

	bodyID_t CreateAndAddBody( const bodyCreationSettings_t &settings, shapeHandle_t shape, void *userData );
	void RemoveAndDestroyBody( bodyID_t bodyID );

	void GetBodyPositionAndRotation( bodyID_t bodyID, vec3_t position, vec3_t rotation );

	void AddLinearVelocity( bodyID_t bodyID, vec3_t velocity );
	void SetLinearAndAngularVelocity( bodyID_t bodyID, vec3_t velocity, vec3_t avelocity );
	void GetLinearAndAngularVelocity( bodyID_t bodyID, vec3_t velocity, vec3_t avelocity );

	JPH::PhysicsSystem *GetPhysicsSystem();

	void AddWorld( JPH::VertexList &vertexList, JPH::IndexedTriangleList &indexList );
}

extern IPhysicsSystem *g_physSystem;
