//=================================================================================================
// OS-dependent input for external (non-keyboard) input devices
//=================================================================================================

#pragma once

namespace input
{
	void Init();

	void Shutdown();

	// oportunity for devices to stick commands on the script buffer
	void Commands();

	void Frame();

	void Activate( bool active );
}
