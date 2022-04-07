
#include "phys_local.h"

#include "phys_body.h"

namespace PhysicsPrivate {

void CPhysicsBody::GetPositionAndRotation( vec3_t position, vec3_t rotation )
{
	JPH::Vec3 jphPosition = m_pBody->GetPosition();
	JPH::Quat jphRotation = m_pBody->GetRotation();

	JoltPositionToQuake( jphPosition, position );
	JoltQuatToQuakeEuler( jphRotation, rotation );
}

//-------------------------------------------------------------------------------------------------

void CPhysicsBody::SetLinearAndAngularVelocity( const vec3_t velocity, const vec3_t avelocity )
{
	JPH::Vec3 jphLinearVelocity = QuakePositionToJolt( velocity );
	JPH::Vec3 jphAngularVelocity = QuakePositionToJolt( avelocity );

	JPH::BodyInterface &bodyInterface = m_pPhysicsSystem->GetBodyInterfaceNoLock();

	bodyInterface.SetLinearAndAngularVelocity( m_pBody->GetID(), jphLinearVelocity, jphAngularVelocity );
}

void CPhysicsBody::GetLinearAndAngularVelocity( vec3_t velocity, vec3_t avelocity )
{
	JPH::BodyInterface &bodyInterface = m_pPhysicsSystem->GetBodyInterfaceNoLock();

	JPH::Vec3 jphLinearVelocity, jphAngularVelocity;

	bodyInterface.GetLinearAndAngularVelocity( m_pBody->GetID(), jphLinearVelocity, jphAngularVelocity );

	jphLinearVelocity *= JoltToQuakeFactor;
	jphAngularVelocity *= JoltToQuakeFactor;

	jphLinearVelocity.StoreFloat3( (JPH::Float3 *)velocity );
	jphAngularVelocity.StoreFloat3( (JPH::Float3 *)avelocity );
}

void CPhysicsBody::AddLinearVelocity( const vec3_t velocity )
{
	JPH::Vec3 jphLinearVelocity = QuakePositionToJolt( velocity );

	JPH::BodyInterface &bodyInterface = m_pPhysicsSystem->GetBodyInterfaceNoLock();

	bodyInterface.AddLinearVelocity( m_pBody->GetID(), jphLinearVelocity );
}

//-------------------------------------------------------------------------------------------------

const IPhysicsShape *CPhysicsBody::GetShape()
{
	return reinterpret_cast<const IPhysicsShape *>( m_pBody->GetShape() );
}

} // namespace PhysicsPrivate
