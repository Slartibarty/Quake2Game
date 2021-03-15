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

#define	MAXTOUCH	32
struct pmove_t
{
	// state (in / out)
	pmove_state_t	s;

	// command (in)
	usercmd_t	cmd;

	// results (out)
	int			numtouch;
	edict_t		*touchents[MAXTOUCH];

	vec3_t		viewangles;			// clamped
	float		viewheight;

	vec3_t		mins, maxs;			// bounding box size

	edict_t		*groundentity;
	int			watertype;
	int			waterlevel;

	// callbacks to test the world
	// these will be different functions during game and cgame
	trace_t		(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
	int			(*pointcontents) (vec3_t point);
	void		(*playsound) (const char *sample, float volume);
};

void Pmove( pmove_t *pmove );
