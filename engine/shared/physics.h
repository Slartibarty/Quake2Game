
#pragma once

#include <Jolt.h>

#include <Physics/PhysicsSystem.h>

#include <Geometry/IndexedTriangle.h>

#include "../../common/physics_interface.h"

namespace Physics
{
	void Init();
	void Shutdown();
	void Simulate( float deltaTime );

	bodyID_t CreateBodySphere( const vec3_t position, const vec3_t angles, float radius );

	void GetBodyTransform( bodyID_t bodyID, vec3_t position, vec3_t angles );

	void SetLinearAndAngularVelocity( bodyID_t bodyID, vec3_t velocity, vec3_t avelocity );

	JPH::PhysicsSystem *GetPhysicsSystem();

	void AddWorld( const JPH::VertexList &vertexList, const JPH::IndexedTriangleList &indexList );
}

extern IPhysicsSystem *g_physSystem;
