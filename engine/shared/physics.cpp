
#include "engine.h"

namespace PhysicsImpl {

static IPhysicsSystem *s_pSystem;
static IPhysicsScene *s_pScene;

void Init()
{
	s_pSystem = Physics::GetPhysicsSystem();
	s_pSystem->Init();

	s_pScene = s_pSystem->CreateScene();
}

void Shutdown()
{
	s_pSystem->DestroyScene( s_pScene );
	s_pScene = nullptr;

	s_pSystem->Shutdown();
	s_pSystem = nullptr;
}

IPhysicsScene *GetScene()
{
	return s_pScene;
}

}
