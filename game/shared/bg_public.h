/*
===============================================================================

	Definitions shared by both the server game and client game modules

===============================================================================
*/

#pragma once

#include "../../common/q_shared.h"

/*
===============================================================================

	Pmove module

	The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
	and some other output data.  Used for local prediction on the client game and true
	movement on the server game.

===============================================================================
*/

void Pmove( pmove_t *pmove );
