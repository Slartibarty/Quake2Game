//=================================================================================================
// Player movement
//
// This is common between the server and client so prediction matches
//=================================================================================================

#pragma once

#include "../../common/q_shared.h" // pmove_t

extern float pm_airaccelerate;

void Pmove( pmove_t *pmove );
