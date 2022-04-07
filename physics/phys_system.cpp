
#include "phys_local.h"

#include "phys_scene.h"

#include "phys_system.h"

//-------------------------------------------------------------------------------------------------

namespace PhysicsPrivate {

CPhysicsSystem CPhysicsSystem::s_physicsSystem;

void CPhysicsSystem::Init()
{
	// Install callbacks
	JPH::Trace = CPhysicsSystem::OnTrace;
	JPH_IF_ENABLE_ASSERTS( JPH::AssertFailed = CPhysicsSystem::OnAssert; )

	// Register all Jolt physics types
	JPH::RegisterTypes();

	// We need a temp allocator for temporary allocations during the physics update. We're
	// pre-allocating 10 MB to avoid having to do allocations during the physics update. 
	// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
	// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
	// malloc / free.
	m_pTempAllocator = new JPH::TempAllocatorImpl( cTempAllocSize );

	// We need a job system that will execute physics jobs on multiple threads. Typically
	// you would implement the JobSystem interface yourself and let Jolt Physics run on top
	// of your own job scheduler. JobSystemThreadPool is an example implementation.
	m_pJobSystem = new JPH::JobSystemThreadPool( JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, cNumThreads );
}

void CPhysicsSystem::Shutdown()
{
	delete m_pJobSystem;
	delete m_pTempAllocator;
}

//-------------------------------------------------------------------------------------------------

IPhysicsScene *CPhysicsSystem::CreateScene()
{
	return new CPhysicsScene;
}

void CPhysicsSystem::DestroyScene( IPhysicsScene *pScene )
{
	delete static_cast<CPhysicsScene *>( pScene );
}

//-------------------------------------------------------------------------------------------------

IPhysicsShape *CPhysicsSystem::CreateBoxShape( vec3_t halfExtent )
{
	JPH::BoxShape *pShape = new JPH::BoxShape( QuakePositionToJolt( halfExtent ) );

	return reinterpret_cast<IPhysicsShape *>( pShape );
}

IPhysicsShape *CPhysicsSystem::CreateSphereShape( float radius )
{
	JPH::SphereShape *pShape = new JPH::SphereShape( QuakeToJolt( radius ) );
	
	return reinterpret_cast<IPhysicsShape *>( pShape );
}

void CPhysicsSystem::DestroyShape( IPhysicsShape *pShape )
{
	delete reinterpret_cast<JPH::Shape *>( pShape );
}

//-------------------------------------------------------------------------------------------------

void CPhysicsSystem::OnTrace( const char *fmt, ... )
{
	va_list args;
	char msg[MAX_PRINT_MSG];

	va_start( args, fmt );
	Q_sprintf_s( msg, sizeof( msg ), fmt, args );
	va_end( args );

	Com_Printf( "%s\n", msg );
}

bool CPhysicsSystem::OnAssert( const char *inExpression, const char *inMessage, const char *inFile, uint inLine )
{
	AssertionFailed( inMessage, inExpression, inFile, inLine );

	// Return false to not break
	return false;
}

} // namespace PhysicsPrivate

//-------------------------------------------------------------------------------------------------

namespace Physics { IPhysicsSystem *GetPhysicsSystem() { return PhysicsPrivate::CPhysicsSystem::GetInstance(); } }
