//=================================================================================================
// OS-dependent input for external (non-keyboard) input devices
//=================================================================================================

#pragma once

void IN_Init();

void IN_Shutdown();

// oportunity for devices to stick commands on the script buffer
void IN_Commands();

void IN_Frame();

// add additional movement on top of the keyboard move cmd
void IN_Move(usercmd_t *cmd);

void IN_Activate(qboolean active);
