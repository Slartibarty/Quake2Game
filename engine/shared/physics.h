
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
	void SetContactListener( void *listener );
	void Simulate( float deltaTime );

	void LineTrace( const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, trace_t &inTrace );

	//
	// Shape Creation
	//

	shapeHandle_t CreateBoxShape( vec3_t halfExtent );
	shapeHandle_t CreateCapsuleShape( float halfHeightOfCylinder, float radius );
	shapeHandle_t CreateCylinderShape( float halfHeight, float radius );
	shapeHandle_t CreateSphereShape( float radius );
	shapeHandle_t CreateTaperedCapsuleShape( float halfHeightOfTaperedCylinder, float topRadius, float bottomRadius );
	void DestroyShape( shapeHandle_t handle );

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
