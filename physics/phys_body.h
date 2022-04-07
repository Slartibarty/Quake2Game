
#pragma once

namespace PhysicsPrivate {

class CPhysicsBody final : public IPhysicsBody
{
public:
	CPhysicsBody( JPH::Body *pBody, JPH::PhysicsSystem *pPhysicsSystem )
		: m_pBody( pBody ), m_pPhysicsSystem( pPhysicsSystem ) {}

	void GetPositionAndRotation( vec3_t position, vec3_t rotation ) override;

	void SetLinearAndAngularVelocity( const vec3_t velocity, const vec3_t avelocity ) override;
	void GetLinearAndAngularVelocity( vec3_t velocity, vec3_t avelocity ) override;
	void AddLinearVelocity( const vec3_t velocity ) override;

	const IPhysicsShape *GetShape() override;

public:
	JPH::BodyID GetBodyID() { return m_pBody->GetID(); }
	JPH::Body *GetBody() { return m_pBody; }

private:
	JPH::Body *m_pBody;
	JPH::PhysicsSystem *m_pPhysicsSystem;

};

} // namespace PhysicsPrivate
