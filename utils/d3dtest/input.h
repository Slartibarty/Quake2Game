
#pragma once

void Input_Init();
void Input_ReportCamera();
void Input_Frame();

void HandleKeyboardInput( RAWKEYBOARD &raw );
void HandleMouseInput( RAWMOUSE &raw );

DirectX::XMMATRIX GetViewMatrix();

LRESULT CALLBACK D3DTestWindowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam );
