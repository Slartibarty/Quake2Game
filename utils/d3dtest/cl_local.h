/*
===================================================================================================

	Base header for client modules

===================================================================================================
*/

#pragma once

#include "engine.h"

// This shouldn't be here
#include "../../thirdparty/DirectXMath/Inc/DirectXMath.h"

#include "renderer/d3d_public.h"

#include "cl_public.h"

//
// cl_input.cpp
//

void	Input_KeyEvent( uint32 key, bool down );
uint32	Input_GetKeyState( uint32 key );

void	Input_MouseEvent( int32 lastX, int32 lastY );

void	Input_Frame();

//
// cl_dev.cpp
//

DirectX::XMMATRIX CL_BuildViewMatrix();

void CL_DevFrame();

//
// sys_input.cpp
//

void SysInput_Init();

//
// sys_mainwnd.cpp
//

extern StaticCvar wnd_width;
extern StaticCvar wnd_height;

void	Sys_CreateMainWindow();
void	Sys_DestroyMainWindow();
void	Sys_ShowMainWindow();
void *	Sys_GetMainWindow();
