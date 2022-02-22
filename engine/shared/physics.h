
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

	trace_t LineTrace( vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headnode, int brushmask );

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

	void GetBodyPositionAndRotation( bodyID_t bodyID, vec3_t position, vec3_t rotation );

	void AddLinearVelocity( bodyID_t bodyID, vec3_t velocity );
	void SetLinearAndAngularVelocity( bodyID_t bodyID, vec3_t velocity, vec3_t avelocity );

	JPH::PhysicsSystem *GetPhysicsSystem();

	void AddWorld( JPH::VertexList &vertexList, JPH::IndexedTriangleList &indexList );
}

extern IPhysicsSystem *g_physSystem;
