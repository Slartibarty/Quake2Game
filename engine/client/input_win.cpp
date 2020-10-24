//=================================================================================================
// Windows mouse code
//=================================================================================================

#include "client.h"
#include "winquake.h"

extern	unsigned	sys_msg_time;

cvar_t *in_mouse;

bool	in_appactive;


//-------------------------------------------------------------------------------------------------
// Mouse control
//-------------------------------------------------------------------------------------------------

// mouse variables
cvar_t *m_filter;

qboolean	mlooking;

inline void IN_MLookDown()
{
	mlooking = true;
}

inline void IN_MLookUp(void)
{
	mlooking = false;
	if (!freelook->value && lookspring->value)
		IN_CenterView();
}

int			mouse_buttons;
int			mouse_oldbuttonstate;
POINT		current_pos;
int			mouse_x, mouse_y, old_mouse_x, old_mouse_y, mx_accum, my_accum;

int			old_x, old_y;

qboolean	mouseactive;	// false when not focus app

qboolean	restore_spi;
qboolean	mouseinitialized;
int		originalmouseparms[3], newmouseparms[3] = { 0, 0, 0 };
qboolean	mouseparmsvalid;

int			window_center_x, window_center_y;
RECT		window_rect;


/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
void IN_ActivateMouse(void)
{
	int		width, height;

	if (!mouseinitialized)
		return;
	if (!in_mouse->value)
	{
		mouseactive = false;
		return;
	}
	if (mouseactive)
		return;

	mouseactive = true;

	if (mouseparmsvalid)
		restore_spi = SystemParametersInfo(SPI_SETMOUSE, 0, newmouseparms, 0);

	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);

	GetWindowRect(cl_hwnd, &window_rect);
	if (window_rect.left < 0)
		window_rect.left = 0;
	if (window_rect.top < 0)
		window_rect.top = 0;
	if (window_rect.right >= width)
		window_rect.right = width - 1;
	if (window_rect.bottom >= height - 1)
		window_rect.bottom = height - 1;

	window_center_x = (window_rect.right + window_rect.left) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;

	SetCursorPos(window_center_x, window_center_y);

	old_x = window_center_x;
	old_y = window_center_y;

	SetCapture(cl_hwnd);
	ClipCursor(&window_rect);
	while (ShowCursor(FALSE) >= 0)
		;
}


/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
void IN_DeactivateMouse(void)
{
	if (!mouseinitialized)
		return;
	if (!mouseactive)
		return;

	if (restore_spi)
		SystemParametersInfo(SPI_SETMOUSE, 0, originalmouseparms, 0);

	mouseactive = false;

	ClipCursor(NULL);
	ReleaseCapture();
	while (ShowCursor(TRUE) < 0)
		;
}



/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse(void)
{
	cvar_t *cv;

	cv = Cvar_Get("in_initmouse", "1", CVAR_NOSET);
	if (!cv->value)
		return;

	mouseinitialized = true;
	mouseparmsvalid = SystemParametersInfo(SPI_GETMOUSE, 0, originalmouseparms, 0);
	mouse_buttons = 3;
}

/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent(int mstate)
{
	int		i;

	if (!mouseinitialized)
		return;

	// perform button actions
	for (i = 0; i < mouse_buttons; i++)
	{
		if ((mstate & (1 << i)) &&
			!(mouse_oldbuttonstate & (1 << i)))
		{
			Key_Event(K_MOUSE1 + i, true, sys_msg_time);
		}

		if (!(mstate & (1 << i)) &&
			(mouse_oldbuttonstate & (1 << i)))
		{
			Key_Event(K_MOUSE1 + i, false, sys_msg_time);
		}
	}

	mouse_oldbuttonstate = mstate;
}


/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove(usercmd_t *cmd)
{
	int		mx, my;

	if (!mouseactive)
		return;

	// find mouse movement
	if (!GetCursorPos(&current_pos))
		return;

	mx = current_pos.x - window_center_x;
	my = current_pos.y - window_center_y;

	if (m_filter->value)
	{
		mouse_x = (mx + old_mouse_x) * 0.5;
		mouse_y = (my + old_mouse_y) * 0.5;
	}
	else
	{
		mouse_x = mx;
		mouse_y = my;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

	mouse_x *= sensitivity->value;
	mouse_y *= sensitivity->value;

	// add mouse X/Y movement to cmd
	if ((in_strafe.state & 1) || (lookstrafe->value && mlooking))
		cmd->sidemove += m_side->value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw->value * mouse_x;

	if ((mlooking || freelook->value) && !(in_strafe.state & 1))
	{
		cl.viewangles[PITCH] += m_pitch->value * mouse_y;
	}
	else
	{
		cmd->forwardmove -= m_forward->value * mouse_y;
	}

	// force the mouse to the center, so there's room to move
	if (mx || my)
		SetCursorPos(window_center_x, window_center_y);
}


//-------------------------------------------------------------------------------------------------
// Public
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void IN_Init(void)
{
	// mouse variables
	m_filter = Cvar_Get("m_filter", "0", 0);
	in_mouse = Cvar_Get("in_mouse", "1", CVAR_ARCHIVE);

	Cmd_AddCommand("+mlook", IN_MLookDown);
	Cmd_AddCommand("-mlook", IN_MLookUp);

	IN_StartupMouse();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void IN_Shutdown(void)
{
	IN_DeactivateMouse();
}

//-------------------------------------------------------------------------------------------------
// Called every frame, even if not generating commands
//-------------------------------------------------------------------------------------------------
void IN_Frame(void)
{
	if (!mouseinitialized)
		return;

	if (!in_mouse || !in_appactive)
	{
		IN_DeactivateMouse();
		return;
	}

	if (!cl.refresh_prepped
		|| cls.key_dest == key_console
		|| cls.key_dest == key_menu)
	{
		// temporarily deactivate if in fullscreen
		if (Cvar_VariableValue("vid_fullscreen") == 0)
		{
			IN_DeactivateMouse();
			return;
		}
	}

	IN_ActivateMouse();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void IN_Commands()
{

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void IN_Move(usercmd_t *cmd)
{
	IN_MouseMove(cmd);
}

//-------------------------------------------------------------------------------------------------
// Called when the main window gains or loses focus.
// The window may have been destroyed and recreated
// between a deactivate and an activate.
//-------------------------------------------------------------------------------------------------
void IN_Activate(qboolean active)
{
	in_appactive = active;
	mouseactive = !active;		// force a new window check or turn off
}
