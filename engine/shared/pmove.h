//=================================================================================================
// Player movement
//
// This is common between the server and client so prediction matches
//=================================================================================================

#pragma once

#include "../../common/q_shared.h" // pmove_t

void Pmove( pmove_t *pmove );
