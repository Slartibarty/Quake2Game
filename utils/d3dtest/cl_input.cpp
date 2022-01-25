/*
===================================================================================================

	Client Input

	This is wher

===================================================================================================
*/

#include "cl_local.h"

struct clKey_t
{
	bool down;
};

static struct clInput_t
{
	clKey_t keys[256];

	int32 lastXPos, lastYPos; // Mouse guff
	int32 curXPos, curYPos; // Mouse guff
} clInput;

static StaticCvar m_sensitivity( "m_sensitivity", "3", CVAR_ARCHIVE );
static StaticCvar m_pitch( "m_pitch", "0.022", CVAR_ARCHIVE );
static StaticCvar m_yaw( "m_yaw", "0.022", CVAR_ARCHIVE );

void Input_KeyEvent( uint32 key, bool down )
{
	if ( key > 255 )
	{
		assert( 0 );
		return;
	}

	clInput.keys[key].down = down;
}

uint32 Input_GetKeyState( uint32 key )
{
	assert( key <= 255 );

	uint32 amount = static_cast<uint32>( clInput.keys[key].down );

	return amount;
}

void Input_MouseEvent( int32 lastX, int32 lastY )
{
	clInput.curXPos = ( lastX + clInput.lastXPos ) * m_yaw.GetFloat() * m_sensitivity.GetFloat();
	clInput.curYPos = ( lastY + clInput.lastYPos ) * m_pitch.GetFloat() * m_sensitivity.GetFloat();
	clInput.lastXPos = lastX;
	clInput.lastYPos = lastY;

	//clInput.camera.angles.y -= xpos; // Yaw
	//clInput.camera.angles.x += ypos; // Pitch
}

void Input_Frame()
{

}
