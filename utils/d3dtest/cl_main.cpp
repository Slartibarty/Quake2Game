
#include "cl_local.h"

void CL_Init()
{
	// Create the window, we need this to init raw input
	Sys_CreateMainWindow();

	// Register raw input devices
	SysInput_Init();

	Renderer::Init();
}

void CL_Shutdown()
{
	Renderer::Shutdown();

	Sys_DestroyMainWindow();
}

void CL_Frame( float deltaTime )
{
	CL_DevFrame();

	Renderer::Frame();
}
