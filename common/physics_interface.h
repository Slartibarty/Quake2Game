/*
===================================================================================================

	Physics engine interface

===================================================================================================
*/

#pragma once

using bodyID_t = uint32;

abstract_class IPhysicsSystem
{
public:
	virtual void		Simulate( float deltaTime ) = 0;
	virtual bodyID_t	CreateBodySphere( const vec3_t position, const vec3_t angles, float radius ) = 0;

	virtual void		GetBodyTransform( bodyID_t bodyID, vec3_t position, vec3_t angles ) = 0;

	virtual void		SetLinearAndAngularVelocity( bodyID_t bodyID, vec3_t velocity, vec3_t avelocity ) = 0;
};
