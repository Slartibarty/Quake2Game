
#pragma once

#include <Jolt.h>

#include <Physics/PhysicsSystem.h>

#include <Geometry/IndexedTriangle.h>

#include "../../common/physics_interface.h"

namespace Physics
{
	//
	// Internals
	//

	void Init();
	void Shutdown();
	void Simulate( float deltaTime );

	//
	// Shape Creation
	//

	shapeHandle_t CreateBoxShape( vec3_t halfExtent );
	shapeHandle_t CreateSphereShape( float radius );
	void DestroyShape( shapeHandle_t handle );

	//
	// Bodies
	//

	bodyID_t CreateAndAddBody( const bodyCreationSettings_t &settings, shapeHandle_t shape );
	void RemoveAndDestroyBody( bodyID_t bodyID );

	void GetBodyPositionAndRotation( bodyID_t bodyID, vec3_t position, vec3_t angles );

	void SetLinearAndAngularVelocity( bodyID_t bodyID, vec3_t velocity, vec3_t avelocity );

	JPH::PhysicsSystem *GetPhysicsSystem();

	void AddWorld( const JPH::VertexList &vertexList, const JPH::IndexedTriangleList &indexList );
}

extern IPhysicsSystem *g_physSystem;
