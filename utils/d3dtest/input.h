
#pragma once

#include "../../thirdparty/DirectXMath/Inc/DirectXMath.h"

void Input_Init();
void Input_Frame();

DirectX::XMMATRIX GetViewMatrix();

LRESULT CALLBACK D3DTestWindowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam );
