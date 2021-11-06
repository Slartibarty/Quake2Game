/*
===================================================================================================

	Terrible Input

	Notes on coordinates:

	A completely zeroed camera is looking down positive X

	Origin:
		Positive X is moving forwards
		Negative X is moving backwards
		Positive Y is moving left
		Negative Y is moving right
		Positive Z is moving up
		Negative Z is moving down

	Angles:
		

===================================================================================================
*/

#include "../../core/core.h"

#include "../../framework/cvarsystem.h"

#include "../../core/sys_includes.h"

// Local
#include "window.h"

// This
#include "input.h"

static constexpr float pm_maxspeed = 320;
static constexpr float pm_accelerate = 10;
static constexpr float pm_noclipfriction = 12;

float sys_frameTime;

struct Camera
{
	DirectX::XMFLOAT3 origin;
	DirectX::XMFLOAT3 angles;
};

static struct inputDefs_t
{
	Camera camera;
	RECT windowRect;
	LONG lastXPos, lastYPos;
	bool appActive;
	bool mouseActive;
} input;

static Camera s_Camera;

static StaticCvar m_sensitivity( "m_sensitivity", "3", CVAR_ARCHIVE );
static StaticCvar m_pitch( "m_pitch", "0.022", CVAR_ARCHIVE );
static StaticCvar m_yaw( "m_yaw", "0.022", CVAR_ARCHIVE );

void Input_Init()
{
	input.camera.origin.x = 0.0f;
	input.camera.origin.y = 0.0f;
	input.camera.origin.z = 8.0f;

	input.camera.angles.x = 0.0f;
	input.camera.angles.y = 0.0f;

	// Register for raw input

	Com_Print( "Initialising Windows InputSystem...\n" );

	RAWINPUTDEVICE rid[2];

	// Mouse
	rid[0].usUsagePage = 0x01;
	rid[0].usUsage = 0x02;
	rid[0].dwFlags = 0;
	rid[0].hwndTarget = (HWND)GetMainWindow();

	// Keyboard
	rid[1].usUsagePage = 0x01;
	rid[1].usUsage = 0x06;
	rid[1].dwFlags = 0;
	rid[1].hwndTarget = (HWND)GetMainWindow();

	BOOL result = RegisterRawInputDevices( rid, 2, sizeof( *rid ) );
	if ( result == FALSE ) {
		Com_FatalError( "Failed to register for raw input!\n" );
	}
}

static void Input_ReportCamera()
{
	Com_Printf( "Origin: %.6f %.6f %.6f\n", input.camera.origin.x, input.camera.origin.y, input.camera.origin.z );
	Com_Printf( "Angles: %.6f %.6f %.6f\n", input.camera.angles.x, input.camera.angles.y, input.camera.angles.z );
}

struct userCmd_t
{
	float forwardMove, leftMove, upMove;
};

static vec3_t s_velocity;
static userCmd_t s_cmd;

/*
===================
PM_Accelerate

Handles user intended acceleration
===================
*/
static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel )
{
	int			i;
	float		addspeed, accelspeed, currentspeed;

	// See if we are changing direction a bit
	currentspeed = DotProduct( s_velocity, wishdir );

	// Reduce wishspeed by the amount of veer
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done
	if ( addspeed <= 0.0f ) {
		return;
	}

	// Determine amount of accleration
	accelspeed = accel * sys_frameTime * wishspeed;

	// Cap at addspeed
	if ( accelspeed > addspeed ) {
		accelspeed = addspeed;
	}

	// Adjust velocity
	for ( i = 0; i < 3; i++ )
	{
		s_velocity[i] += accelspeed * wishdir[i];
	}
}

/*
===================
PM_NoclipMove
===================
*/
static void PM_NoclipMove( userCmd_t &cmd )
{
	int			i;
	float		speed, drop, friction, newspeed, stopspeed;
	float		wishspeed;
	vec3_t		wishdir;

	vec3_t forward, right, up;
	AngleVectors( (float *)&input.camera.angles, forward, right, up );

	// friction
	speed = VectorLength( s_velocity );
	if ( speed < 20.0f ) {
		VectorClear( s_velocity );
	}
	else {
		stopspeed = pm_maxspeed * 0.3f;
		if ( speed < stopspeed ) {
			speed = stopspeed;
		}
		friction = pm_noclipfriction;
		drop = speed * friction * sys_frameTime;

		// scale the velocity
		newspeed = speed - drop;
		if ( newspeed < 0 ) {
			newspeed = 0;
		}

		VectorScale( s_velocity, newspeed / speed, s_velocity );
	}

	// accelerate

	for ( i = 0; i < 3; ++i ) {
		wishdir[i] = forward[i] * cmd.forwardMove + right[i] * cmd.leftMove;
	}
	wishdir[2] += cmd.upMove;

	wishspeed = VectorNormalize( wishdir );

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	input.camera.origin.x += sys_frameTime * s_velocity[0];
	input.camera.origin.y += sys_frameTime * s_velocity[1];
	input.camera.origin.z += sys_frameTime * s_velocity[2];
}

void Input_Frame()
{
	PM_NoclipMove( s_cmd );
	memset( &s_cmd, 0, sizeof( s_cmd ) );

	//Input_ReportCamera();
}

void HandleKeyboardInput( RAWKEYBOARD &raw )
{
	switch ( raw.VKey )
	{
	case 'A':
		s_cmd.leftMove -= pm_maxspeed;
		break;
	case 'W':
		s_cmd.forwardMove += pm_maxspeed;
		break;
	case 'D':
		s_cmd.leftMove += pm_maxspeed;
		break;
	case 'S':
		s_cmd.forwardMove -= pm_maxspeed;
		break;
	}
}

void HandleMouseInput( RAWMOUSE &raw, HWND wnd )
{
	// Always process BUTTONS!
	switch ( raw.usButtonFlags )
	{
	case RI_MOUSE_LEFT_BUTTON_DOWN:
		//Key_Event( K_MOUSE1, true, sys_frameTime );
		break;
	case RI_MOUSE_LEFT_BUTTON_UP:
		//Key_Event( K_MOUSE1, false, sys_frameTime );
		break;
	case RI_MOUSE_RIGHT_BUTTON_DOWN:
		//Key_Event( K_MOUSE2, true, sys_frameTime );
		input.mouseActive = true;
		SetCapture( wnd );
		ClipCursor( &input.windowRect );
		while ( ShowCursor( FALSE ) >= 0 )
			;
		break;
	case RI_MOUSE_RIGHT_BUTTON_UP:
		//Key_Event( K_MOUSE2, false, sys_frameTime );
		input.mouseActive = false;
		ClipCursor( nullptr );
		ReleaseCapture();
		while ( ShowCursor( TRUE ) < 0 )
			;
		break;
	case RI_MOUSE_MIDDLE_BUTTON_DOWN:
		//Key_Event( K_MOUSE3, true, sys_frameTime );
		break;
	case RI_MOUSE_MIDDLE_BUTTON_UP:
		//Key_Event( K_MOUSE3, false, sys_frameTime );
		break;
	case RI_MOUSE_WHEEL:
		//Key_Event( ( (short)raw.usButtonData > 0 ) ? K_MWHEELUP : K_MWHEELDOWN, true, sys_frameTime );
		//Key_Event( ( (short)raw.usButtonData > 0 ) ? K_MWHEELUP : K_MWHEELDOWN, false, sys_frameTime );
		break;
	}

	if ( input.mouseActive == false ) {
		return;
	}

	float xpos = static_cast<float>( raw.lLastX + input.lastXPos );
	float ypos = static_cast<float>( raw.lLastY + input.lastYPos );
	input.lastXPos = raw.lLastX;
	input.lastYPos = raw.lLastY;

	xpos *= m_yaw.GetFloat() * m_sensitivity.GetFloat();
	ypos *= m_pitch.GetFloat() * m_sensitivity.GetFloat();

#if 0
	// force the mouse to the center, so there's room to move
	if ( mx || my )
		SetCursorPos( window_center_x, window_center_y );
#endif

	input.camera.angles.y -= xpos; // Yaw
	input.camera.angles.x += ypos; // Pitch
}

DirectX::XMMATRIX GetViewMatrix()
{
	using namespace DirectX;

	vec3 viewOrg = std::bit_cast<vec3>( input.camera.origin );
	vec3 viewAng = std::bit_cast<vec3>( input.camera.angles );

	// Invert pitch
	viewAng[PITCH] = -viewAng[PITCH];

	vec3 forward;
	forward.x = cos( DEG2RAD( viewAng[YAW] ) ) * cos( DEG2RAD( viewAng[PITCH] ) );
	forward.y = sin( DEG2RAD( viewAng[YAW] ) ) * cos( DEG2RAD( viewAng[PITCH] ) );
	forward.z = sin( DEG2RAD( viewAng[PITCH] ) );
	forward.NormalizeFast();

	XMVECTOR eyePosition = XMLoadFloat3( (XMFLOAT3 *)&viewOrg );
	XMVECTOR eyeDirection = XMLoadFloat3( (XMFLOAT3 *)&forward );
	XMVECTOR upDirection = XMVectorSet( 0.0f, 0.0f, 1.0f, 0.0f );

	return XMMatrixLookToRH( eyePosition, eyeDirection, upDirection );
}

LRESULT CALLBACK D3DTestWindowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
	case WM_ERASEBKGND:
		return 0;
	case WM_CLOSE:
		PostQuitMessage( EXIT_SUCCESS );
		return 0;
	case WM_INPUT:
	{
		UINT dwSize = sizeof( RAWINPUT );

		RAWINPUT raw;

		// Now read into the buffer
		GetRawInputData( (HRAWINPUT)lParam, RID_INPUT, &raw, &dwSize, sizeof( RAWINPUTHEADER ) );

		// if imgui is focused, we should not send keyboard input to the game
		switch ( raw.header.dwType )
		{
		case RIM_TYPEKEYBOARD:
			HandleKeyboardInput( raw.data.keyboard );
			break;
		case RIM_TYPEMOUSE:
			HandleMouseInput( raw.data.mouse, wnd );
			break;
		}
	}
	return 0;
	case WM_SIZE: [[fallthrough]];
	case WM_MOVE:
		GetClientRect( wnd, &input.windowRect );
		MapWindowPoints( wnd, nullptr, (LPPOINT)&input.windowRect, 2 );
		return 0;
	}

	return DefWindowProcW( wnd, msg, wParam, lParam );
}
