
#pragma once

namespace PhysicsPrivate {

class CPhysicsSystem final : public IPhysicsSystem
{
public:
	void Init() override;
	void Shutdown() override;

	IPhysicsScene *CreateScene() override;
	void DestroyScene( IPhysicsScene *pScene ) override;

	IPhysicsShape *CreateBoxShape( vec3_t halfExtent ) override;
	IPhysicsShape *CreateSphereShape( float radius ) override;
	void DestroyShape( IPhysicsShape *pShape ) override;

public:
	static CPhysicsSystem *GetInstance() { return &s_physicsSystem; };
	JPH::TempAllocator *GetTempAllocator() { return m_pTempAllocator; };
	JPH::JobSystem *GetJobSystem() { return m_pJobSystem; };

private:
	static constexpr uint cTempAllocSize = 64 * 1024 * 1024;
	static constexpr int cNumThreads = -1;

	static void OnTrace( const char *fmt, ... );
	static bool OnAssert( const char *inExpression, const char *inMessage, const char *inFile, uint inLine );

	static CPhysicsSystem s_physicsSystem;

	JPH::TempAllocator *m_pTempAllocator;
	JPH::JobSystem *m_pJobSystem;

};

} // namespace PhysicsPrivate
