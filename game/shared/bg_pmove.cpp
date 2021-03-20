/*
===============================================================================

	Player movement simulation code

===============================================================================
*/

#include "bg_local.h"

#define	MIN_STEP_NORMAL		0.7f	// can't step up onto very steep slopes
#define	STEPSIZE			18

#define PLAYER_HULLSIZE		16		// 32 units wide and deep

#define GIB_VIEWHEIGHT		-8
#define GIB_HEIGHT			16

#define CROUCH_VIEWHEIGHT	12
#define CROUCH_HEIGHT		18

#define STAND_VIEWHEIGHT	28
#define STAND_HEIGHT		36

// SlartTodo: This should really be dependent on gravity
#define JUMP_VELOCITY		268.3281573f	// sqrt(2 * 800 * 45.0)
#define NON_JUMP_VELOCITY	140.0f
#define AIR_MAX_WISHSPEED	30.0f

// This is 1.0 in Quake 2 and Half-Life 2, 1.001f in Quake 3
#define OVERCLIP			1.0f

#define PM_SURFTYPE_LADDER	1000
#define PM_SURFTYPE_WADE	1001
#define PM_SURFTYPE_SLOSH	1002

// Movement parameters
static constexpr float	pm_stopspeed = 100;
static constexpr float	pm_maxspeed = 320;
static constexpr float	pm_duckspeed = 100;

static constexpr float	pm_waterspeed = pm_maxspeed;

static constexpr float	pm_accelerate = 10;
static constexpr float	pm_airaccelerate = 10;
static constexpr float	pm_wateraccelerate = 10;
static constexpr float	pm_flyaccelerate = 8;

static constexpr float	pm_friction = 4;
static constexpr float	pm_waterfriction = 1;
static constexpr float	pm_noclipfriction = 12;

// This structure is cleared before every pmove
struct pml_t
{
	vec3_t		origin;
	vec3_t		velocity;

	vec3_t		forward, right, up;
	float		frameTime;

	trace_t		groundTrace;

	vec3_t		previousOrigin;
	bool		ladder;
	bool		walking;
};

static pmove_t *	pm;
static pml_t		pml;

//=============================================================================

static int HackRandom( int low, int high )
{
	int range;

	range = high - low + 1;
	if ( !( range - 1 ) )
	{
		return low;
	}
	else
	{
		int offset;
		int rnum;

		rnum = rand();

		offset = rnum % range;

		return ( low + offset );
	}
}

/*
===================
PM_PlayStepSound
===================
*/
static void PM_PlayStepSound( int type, float fvol )
{
	static int iSkipStep;	// SlartTodo: This is the ultimate suck, move to player state
	int irand;

	pm->s.step_left = !pm->s.step_left;

	irand = HackRandom( 0, 1 ) + ( pm->s.step_left * 2 );

	// irand - 0,1 for right foot, 2,3 for left foot
	// used to alternate left and right foot
	// FIXME, move to player state

	// Not handled: Computer, flesh, glass
	switch ( type )
	{
	default:
	case SURFTYPE_CONCRETE:
		switch ( irand )
		{
		// right foot
		case 0:	pm->playsound( "player/footsteps/step1.wav", fvol );	break;
		case 1:	pm->playsound( "player/footsteps/step3.wav", fvol );	break;
		// left foot
		case 2:	pm->playsound( "player/footsteps/step2.wav", fvol );	break;
		case 3:	pm->playsound( "player/footsteps/step4.wav", fvol );	break;
		}
		break;
	case SURFTYPE_METAL:
		switch ( irand )
		{
		// right foot
		case 0:	pm->playsound( "player/footsteps/metal1.wav", fvol );	break;
		case 1:	pm->playsound( "player/footsteps/metal3.wav", fvol );	break;
		// left foot
		case 2:	pm->playsound( "player/footsteps/metal2.wav", fvol );	break;
		case 3:	pm->playsound( "player/footsteps/metal4.wav", fvol );	break;
		}
		break;
	case SURFTYPE_VENT:
		switch ( irand )
		{
		// right foot
		case 0:	pm->playsound( "player/footsteps/duct1.wav", fvol );	break;
		case 1:	pm->playsound( "player/footsteps/duct3.wav", fvol );	break;
		// left foot
		case 2:	pm->playsound( "player/footsteps/duct2.wav", fvol );	break;
		case 3:	pm->playsound( "player/footsteps/duct4.wav", fvol );	break;
		}
		break;
	case SURFTYPE_DIRT:
	case SURFTYPE_GRASS:
	case SURFTYPE_SAND:
	case SURFTYPE_ROCK:
		switch ( irand )
		{
		// right foot
		case 0:	pm->playsound( "player/footsteps/dirt1.wav", fvol );	break;
		case 1:	pm->playsound( "player/footsteps/dirt3.wav", fvol );	break;
		// left foot
		case 2:	pm->playsound( "player/footsteps/dirt2.wav", fvol );	break;
		case 3:	pm->playsound( "player/footsteps/dirt4.wav", fvol );	break;
		}
		break;
	case SURFTYPE_WOOD:
	case SURFTYPE_TILE:
		if ( !HackRandom( 0, 4 ) ) irand = 4;
		switch ( irand )
		{
		// right foot
		case 0:	pm->playsound( "player/footsteps/tile1.wav", fvol );	break;
		case 1:	pm->playsound( "player/footsteps/tile3.wav", fvol );	break;
		// left foot
		case 2:	pm->playsound( "player/footsteps/tile2.wav", fvol );	break;
		case 3:	pm->playsound( "player/footsteps/tile4.wav", fvol );	break;
		case 4: pm->playsound( "player/footsteps/tile5.wav", fvol );	break;
		}
		break;

		//
		// Special fake PM step types
		//

	case PM_SURFTYPE_LADDER:
		switch ( irand )
		{
		// right foot
		case 0:	pm->playsound( "player/footsteps/ladder1.wav", fvol );	break;
		case 1:	pm->playsound( "player/footsteps/ladder3.wav", fvol );	break;
		// left foot
		case 2:	pm->playsound( "player/footsteps/ladder2.wav", fvol );	break;
		case 3:	pm->playsound( "player/footsteps/ladder4.wav", fvol );	break;
		}
		break;
	case PM_SURFTYPE_WADE:
		if ( iSkipStep == 0 )
		{
			iSkipStep++;
			break;
		}

		if ( iSkipStep++ == 3 )
		{
			iSkipStep = 0;
		}

		switch ( irand )
		{
		// right foot
		case 0:	pm->playsound( "player/footsteps/wade1.wav", fvol );	break;
		case 1:	pm->playsound( "player/footsteps/wade2.wav", fvol );	break;
		// left foot
		case 2:	pm->playsound( "player/footsteps/wade3.wav", fvol );	break;
		case 3:	pm->playsound( "player/footsteps/wade4.wav", fvol );	break;
		}
		break;
	case PM_SURFTYPE_SLOSH:
		switch ( irand )
		{
		// right foot
		case 0:	pm->playsound( "player/footsteps/slosh1.wav", fvol );	break;
		case 1:	pm->playsound( "player/footsteps/slosh3.wav", fvol );	break;
		// left foot
		case 2:	pm->playsound( "player/footsteps/slosh2.wav", fvol );	break;
		case 3:	pm->playsound( "player/footsteps/slosh4.wav", fvol );	break;
		}
		break;
	}
}

/*
===================
PM_UpdateStepSound
===================
*/
static void PM_UpdateStepSound()
{
	if ( pm->s.time_step_sound > 0 )
		return;

	float speed = VectorLength( pml.velocity );

	float velwalk;
	float velrun;
	float flduck;

	// UNDONE: need defined numbers for run, walk, crouch, crouch run velocities!!!!	
	if ( pm->s.pm_flags & PMF_DUCKED || pml.ladder )
	{
		velwalk = 60;		// These constants should be based on cl_movespeedkey * cl_forwardspeed somehow
		velrun = 80;		// UNDONE: Move walking to server
		flduck = 100;
	}
	else
	{
		velwalk = 90;
		velrun = 300;
		flduck = 0;
	}

	// to play step sounds, we must be fast enough or on a ladder
	if ( speed < velwalk || !( pml.ladder || pm->groundentity ) ) {
		return;
	}

	bool walking = speed < velrun;

	/*if ( walking ) {
		Com_Printf( "Walking\n" );
	}
	else {
		Com_Printf( "Running\n" );
	}*/

	vec3_t knee;
	vec3_t feet;

	VectorCopy( pml.origin, knee );
	VectorCopy( pml.origin, feet );

	float height = pm->maxs[2] - pm->mins[2];

	knee[2] = pml.origin[2] - 0.3f * height;
	feet[2] = pml.origin[2] - 0.5f * height;

	int step;
	float fvol;

	// find out what we're stepping in or on...
	if ( pml.ladder )
	{
		step = PM_SURFTYPE_LADDER;
		fvol = 0.35f;
		pm->s.time_step_sound = 350;
	}
	else if ( pm->waterlevel == WL_WAIST )
	{
		step = PM_SURFTYPE_WADE;
		fvol = 0.65f;
		pm->s.time_step_sound = 600;
	}
	else if ( pm->waterlevel == WL_FEET )
	{
		step = PM_SURFTYPE_SLOSH;
		fvol = walking ? 0.2f : 0.5f;
		pm->s.time_step_sound = walking ? 400 : 300;
	}
	else
	{
		// find texture under player, if different from current texture, 
		// get material type
		assert( pml.groundTrace.surface );
		step = SurfaceTypeFromFlags( pml.groundTrace.surface->flags );

		switch ( step )
		{
		default:
		case SURFTYPE_CONCRETE:
			fvol = walking ? 0.2f : 0.5f;
			pm->s.time_step_sound = walking ? 400 : 300;
			break;

		case SURFTYPE_METAL:
			fvol = walking ? 0.2f : 0.5f;
			pm->s.time_step_sound = walking ? 400 : 300;
			break;

		case SURFTYPE_VENT:
			fvol = walking ? 0.4f : 0.7f;
			pm->s.time_step_sound = walking ? 400 : 300;
			break;

		case SURFTYPE_DIRT:
		case SURFTYPE_GRASS:
		case SURFTYPE_SAND:
		case SURFTYPE_ROCK:
			fvol = walking ? 0.25f : 0.55f;
			pm->s.time_step_sound = walking ? 400 : 300;
			break;

		case SURFTYPE_WOOD:
			fvol = walking ? 0.25f : 0.55f;
			pm->s.time_step_sound = walking ? 400 : 300;
			break;

		case SURFTYPE_TILE:
		case SURFTYPE_COMPUTER:
		case SURFTYPE_GLASS:
		case SURFTYPE_FLESH:
			fvol = walking ? 0.2f : 0.5f;
			pm->s.time_step_sound = walking ? 400 : 300;
			break;
		}
	}

	pm->s.time_step_sound += flduck; // slower step time if ducking

	// play the sound
	// 35% volume if ducking
	if ( pm->s.pm_flags & PMF_DUCKED ) {
		fvol *= 0.35f;
	}

	PM_PlayStepSound( step, fvol );
}

//=============================================================================

/*
===================
PM_ClipVelocity

Slide off of the impacting surface
===================
*/
static void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce )
{
	float	backoff;
	float	change;
	int		i;

	backoff = DotProduct( in, normal );

	if ( backoff < 0.0f )
	{
		backoff *= overbounce;
	}
	else
	{
		backoff /= overbounce;
	}

	for ( i = 0; i < 3; i++ )
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
	}
}

/*
==================
PM_SlideMove

Each intersection will try to step over the obstruction instead of
sliding along it.

Returns true if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES	5
static bool PM_SlideMove()
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity, original_velocity;
	vec3_t      new_velocity;
	int			i, j;
	trace_t		trace;
	vec3_t		end;
	float		time_left, allFraction;

	numbumps = 4; // Bump up to four times

	numplanes = 0; // Assume not sliding along any planes
	VectorCopy( pml.velocity, original_velocity );
	VectorCopy( pml.velocity, primal_velocity );

	allFraction = 0;
	time_left = pml.frameTime; // Total time for this movement operation

	for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ )
	{
		if ( !pml.velocity[0] && !pml.velocity[1] && !pml.velocity[2] )
			break;

		// Assume we can move all the way from the current origin to the
		//  end point.
		for ( i = 0; i < 3; i++ )
			end[i] = pml.origin[i] + time_left * pml.velocity[i];

		trace = pm->trace( pml.origin, pm->mins, pm->maxs, end );

		allFraction += trace.fraction;
		// If we started in a solid object, or we were in solid space
		//  the whole way, zero out our velocity and return that we
		//  are blocked by floor and wall.
		if ( trace.allsolid )
		{	// entity is completely trapped in another solid
			VectorClear( pml.velocity );		// Zero out all velocity
			return true;
		}

		// If we moved some portion of the total distance, then
		//  copy the end position into the pml.origin and 
		//  zero the plane counter.
		if ( trace.fraction > 0.0f )
		{	// actually covered some distance
			VectorCopy( trace.endpos, pml.origin );
			VectorCopy( pml.velocity, original_velocity );
			numplanes = 0;
		}

		// If we covered the entire distance, we are done
		//  and can return.
		if ( trace.fraction == 1 )
			break;		// moved the entire distance

	   //if (!trace.ent)
	   //	Com_Error (ERR_FATAL, "PM_PlayerTrace: !trace.ent");

		// Save entity that blocked us (since fraction was < 1.0)
		//  for contact
		// Add it if it's not already in the list!!!
		if ( pm->numtouch < MAXTOUCH && trace.ent )
		{
			pm->touchents[pm->numtouch] = trace.ent;
			pm->numtouch++;
		} // SlartTodo: This block differs

		// Reduce amount of pmove->frametime left by total time left * fraction
		//  that we covered.
		time_left -= time_left * trace.fraction;

		// Did we run out of planes to clip against?
		if ( numplanes >= MAX_CLIP_PLANES )
		{	// this shouldn't really happen
			//  Stop our movement if so.
			VectorClear( pml.velocity );
			assert( 0 );
			return true;
		}

		// Set up next clipping plane
		VectorCopy( trace.plane.normal, planes[numplanes] );
		numplanes++;

		//
		// modify original_velocity so it parallels all of the clip planes
		//
		if ( ( !pm->groundentity ) || ( pm_friction != 1 ) )	// relfect player velocity
		{
			for ( i = 0; i < numplanes; i++ )
			{
				if ( planes[i][2] > MIN_STEP_NORMAL )
				{// floor or slope
					PM_ClipVelocity( original_velocity, planes[i], new_velocity, OVERCLIP );
					VectorCopy( new_velocity, original_velocity );
				}
				else
					PM_ClipVelocity( original_velocity, planes[i], new_velocity, OVERCLIP + /*pmove->movevars->bounce*/ 0 * ( 1.0f - pm_friction ) );
			}

			VectorCopy( new_velocity, pml.velocity );
			VectorCopy( new_velocity, original_velocity );
		}
		else
		{
			for ( i = 0; i < numplanes; i++ )
			{
				PM_ClipVelocity(
					original_velocity,
					planes[i],
					pml.velocity,
					OVERCLIP );
				for ( j = 0; j < numplanes; j++ )
					if ( j != i )
					{
						// Are we now moving against this plane?
						if ( DotProduct( pml.velocity, planes[j] ) < 0 )
							break;	// not ok
					}
				if ( j == numplanes )  // Didn't have to clip, so we're ok
					break;
			}

			// Did we go all the way through plane set
			if ( i != numplanes )
			{
				// go along this plane
				// pmove->velocity is set in clipping call, no need to set again.
			}
			else
			{	// go along the crease
				if ( numplanes != 2 )
				{
					//Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
					VectorClear( pml.velocity );
					//Con_DPrintf("Trapped 4\n");

					return true;
				}
				CrossProduct( planes[0], planes[1], dir );
				d = DotProduct( dir, pml.velocity );
				VectorScale( dir, d, pml.velocity );
			}

			//
			// if original velocity is against the original velocity, stop dead
			// to avoid tiny occilations in sloping corners
			//
			if ( DotProduct( pml.velocity, primal_velocity ) <= 0 )
			{
				//Con_DPrintf("Back\n");
				VectorClear( pml.velocity );
				return true;
			}
		}
	}

	if ( allFraction == 0 )
	{
		VectorClear( pml.velocity );
		//Con_DPrintf( "Don't stick\n" );
		return true;
	}

	/*if (pm->s.pm_time)
	{
		VectorCopy (primal_velocity, pml.velocity);
	}*/

	return ( bumpcount != 0 );
}

/*
==================
PM_StepSlideMove

==================
*/
static void PM_StepSlideMove()
{
	vec3_t		start_o, start_v;
	vec3_t		down_o, down_v;
	trace_t		trace;
	float		down_dist, up_dist;
	//	vec3_t		delta;
	vec3_t		up, down;

	VectorCopy( pml.origin, start_o );
	VectorCopy( pml.velocity, start_v );

	if ( !PM_SlideMove() )
	{
		return;		// we got exactly where we wanted to go first try
	}

	VectorCopy( pml.origin, down_o );
	VectorCopy( pml.velocity, down_v );

	VectorCopy( start_o, up );
	up[2] += STEPSIZE;

	trace = pm->trace( up, pm->mins, pm->maxs, up );
	if ( trace.allsolid )
		return;		// can't step up

	// try sliding above
	VectorCopy( up, pml.origin );
	VectorCopy( start_v, pml.velocity );

	PM_SlideMove();

	// push down the final amount
	VectorCopy( pml.origin, down );
	down[2] -= STEPSIZE;
	trace = pm->trace( pml.origin, pm->mins, pm->maxs, down );
	if ( !trace.allsolid )
	{
		VectorCopy( trace.endpos, pml.origin );
	}

#if 0
	VectorSubtract( pml.origin, up, delta );
	up_dist = DotProduct( delta, start_v );

	VectorSubtract( down_o, start_o, delta );
	down_dist = DotProduct( delta, start_v );
#else
	VectorCopy( pml.origin, up );

	// decide which one went farther
	down_dist = ( down_o[0] - start_o[0] ) * ( down_o[0] - start_o[0] )
		+ ( down_o[1] - start_o[1] ) * ( down_o[1] - start_o[1] );
	up_dist = ( up[0] - start_o[0] ) * ( up[0] - start_o[0] )
		+ ( up[1] - start_o[1] ) * ( up[1] - start_o[1] );
#endif

	if ( down_dist > up_dist || trace.plane.normal[2] < MIN_STEP_NORMAL )
	{
		VectorCopy( down_o, pml.origin );
		VectorCopy( down_v, pml.velocity );
		return;
	}
	//!! Special case
	// if we were walking along a plane, then we need to copy the Z over
	pml.velocity[2] = down_v[2];
}


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction()
{
	float	speed, newspeed, control;
	float	friction;
	float	drop;

	// Calculate speed
	speed = VectorLength( pml.velocity );

	// If too slow, return
	if ( speed < 0.1f )
	{
		return;
	}

	drop = 0.0f;

	// apply ground friction
	if ( pm->groundentity /*&& pml.groundsurface && !( pml.groundsurface->flags & SURF_SLICK )*/ )
	{
		friction = pm_friction;

		// Bleed off some speed, but if we have less than the bleed
		//  threshold, bleed the threshold amount

		control = speed < pm_stopspeed ? pm_stopspeed : speed;

		// Add the amount to the drop amount
		drop += control * friction * pml.frameTime;
	}

	// Apply water friction
	if ( pm->waterlevel )
	{
		drop += speed * pm_waterfriction * pm->waterlevel * pml.frameTime;
	}

	// Scale the velocity
	newspeed = speed - drop;
	if ( newspeed < 0.0f )
	{
		newspeed = 0.0f;
	}

	if ( newspeed != speed )
	{
		// Determine proportion of old speed we are using
		newspeed /= speed;
		// Adjust velocity according to proportion
		VectorScale( pml.velocity, newspeed, pml.velocity );
	}
}


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
	currentspeed = DotProduct( pml.velocity, wishdir );

	// Reduce wishspeed by the amount of veer
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done
	if ( addspeed <= 0.0f ) {
		return;
	}

	// Determine amount of accleration
	accelspeed = accel * pml.frameTime * wishspeed;

	// Cap at addspeed
	if ( accelspeed > addspeed ) {
		accelspeed = addspeed;
	}

	// Adjust velocity
	for ( i = 0; i < 3; i++ )
	{
		pml.velocity[i] += accelspeed * wishdir[i];
	}
}

/*
==============
PM_AirAccelerate

Handles user intended air acceleration
==============
*/
static void PM_AirAccelerate( vec3_t wishdir, float wishspeed, float accel )
{
	int			i;
	float		addspeed, accelspeed, currentspeed;
	float		wishspd;

	// Cap speed
	wishspd = Min( wishspeed, AIR_MAX_WISHSPEED );

	// Determine veer amount
	currentspeed = DotProduct( pml.velocity, wishdir );

	// See how much to add
	addspeed = wishspd - currentspeed;

	// If not adding any, done
	if ( addspeed <= 0.0f )
		return;

	// Determine acceleration speed after acceleration
	accelspeed = accel * wishspeed * pml.frameTime;

	// Cap it
	accelspeed = Min( accelspeed, addspeed );

	// Adjust pmove vel
	for ( i = 0; i < 3; i++ )
	{
		pml.velocity[i] += accelspeed * wishdir[i];
	}
}

/*
===================
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
===================
*/
static float PM_CmdScale( const usercmd_t &cmd )
{
	float	max;
	float	total;
	float	scale;

	float	forwardmove = cmd.forwardmove;
	float	sidemove = cmd.sidemove;
	float	upmove = cmd.upmove;

	// since the crouch key doubles as downward movement, ignore downward movement when we're on the ground
	// otherwise crouch speed will be lower than specified
	if ( pm->groundentity ) {
		upmove = 0.0f;
	}

	max = fabs( forwardmove );
	if ( fabs( sidemove ) > max ) {
		max = fabs( sidemove );
	}
	if ( fabs( upmove ) > max ) {
		max = fabs( upmove );
	}
	if ( !max ) {
		return 0.0f;
	}

	total = sqrt( forwardmove * forwardmove + sidemove * sidemove + upmove * upmove );
	scale = max * max / ( 127.0f * total );

	return scale;
}

/*
=============
PM_AddCurrents
=============
*/
static void PM_AddCurrents (vec3_t	wishvel)
{
	vec3_t	v;
	float	s;

	//
	// account for ladders
	//

	if (pml.ladder && fabsf(pml.velocity[2]) <= 200)
	{
		if ((pm->viewangles[PITCH] <= -15) && (pm->cmd.forwardmove > 0))
			wishvel[2] = 200;
		else if ((pm->viewangles[PITCH] >= 15) && (pm->cmd.forwardmove > 0))
			wishvel[2] = -200;
		else if (pm->cmd.upmove > 0)
			wishvel[2] = 200;
		else if (pm->cmd.upmove < 0)
			wishvel[2] = -200;
		else
			wishvel[2] = 0;

		// limit horizontal speed when on a ladder
		if (wishvel[0] < -25)
			wishvel[0] = -25;
		else if (wishvel[0] > 25)
			wishvel[0] = 25;

		if (wishvel[1] < -25)
			wishvel[1] = -25;
		else if (wishvel[1] > 25)
			wishvel[1] = 25;
	}


	//
	// add water currents
	//

	if (pm->watertype & MASK_CURRENT)
	{
		VectorClear (v);

		if (pm->watertype & CONTENTS_CURRENT_0)
			v[0] += 1;
		if (pm->watertype & CONTENTS_CURRENT_90)
			v[1] += 1;
		if (pm->watertype & CONTENTS_CURRENT_180)
			v[0] -= 1;
		if (pm->watertype & CONTENTS_CURRENT_270)
			v[1] -= 1;
		if (pm->watertype & CONTENTS_CURRENT_UP)
			v[2] += 1;
		if (pm->watertype & CONTENTS_CURRENT_DOWN)
			v[2] -= 1;

		s = pm_waterspeed;
		if ((pm->waterlevel == 1) && (pm->groundentity))
			s /= 2;

		VectorMA (wishvel, s, v, wishvel);
	}

	//
	// add conveyor belt velocities
	//

	if (pm->groundentity)
	{
		VectorClear (v);

		if (pml.groundTrace.contents & CONTENTS_CURRENT_0)
			v[0] += 1;
		if (pml.groundTrace.contents & CONTENTS_CURRENT_90)
			v[1] += 1;
		if (pml.groundTrace.contents & CONTENTS_CURRENT_180)
			v[0] -= 1;
		if (pml.groundTrace.contents & CONTENTS_CURRENT_270)
			v[1] -= 1;
		if (pml.groundTrace.contents & CONTENTS_CURRENT_UP)
			v[2] += 1;
		if (pml.groundTrace.contents & CONTENTS_CURRENT_DOWN)
			v[2] -= 1;

		VectorMA (wishvel, 100 /* pm->groundentity->speed */, v, wishvel);
	}
}

/*
===================
PM_WaterJumpMove
===================
*/
static void PM_WaterJumpMove()
{

}

/*
===================
PM_WaterMove
===================
*/
static void PM_WaterMove (void)
{
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;

//
// user intentions
//
	for (i=0 ; i<3 ; i++)
		wishvel[i] = pml.forward[i]*pm->cmd.forwardmove + pml.right[i]*pm->cmd.sidemove;

	if (!pm->cmd.forwardmove && !pm->cmd.sidemove && !pm->cmd.upmove)
		wishvel[2] -= 60;		// drift towards bottom
	else
		wishvel[2] += pm->cmd.upmove;

	PM_AddCurrents (wishvel);

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > pm_maxspeed)
	{
		VectorScale (wishvel, pm_maxspeed/wishspeed, wishvel);
		wishspeed = pm_maxspeed;
	}
	wishspeed *= 0.5f;

	PM_Accelerate (wishdir, wishspeed, pm_wateraccelerate);

	PM_StepSlideMove ();
}


/*
===================
PM_AirMove

===================
*/
static void PM_AirMove (void)
{
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		maxspeed;
	float		scale;

	// Copy movement amounts
	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.sidemove;

	scale = PM_CmdScale( pm->cmd );
	
	// Project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	// project the forward and right directions onto the ground plane
	PM_ClipVelocity (pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP );
	PM_ClipVelocity (pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP );
	// Renormalize
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	// Determine x and y parts of velocity
	for (i=0 ; i<2 ; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	// Zero out z part of velocity
	wishvel[2] = 0;

	// Add currents
	PM_AddCurrents (wishvel);

	// Determine maginitude of speed of move
	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir) * scale;

	// clamp to server defined max speed
	maxspeed = (pm->s.pm_flags & PMF_DUCKED) ? pm_duckspeed : pm_maxspeed;

	if (wishspeed > maxspeed)
	{
		VectorScale (wishvel, maxspeed/wishspeed, wishvel);
		wishspeed = maxspeed;
	}
	
	if ( pml.ladder )
	{
		PM_Accelerate (wishdir, wishspeed, pm_accelerate);
		if (!wishvel[2])
		{
			if (pml.velocity[2] > 0)
			{
				pml.velocity[2] -= pm->s.gravity * pml.frameTime;
				if (pml.velocity[2] < 0)
					pml.velocity[2]  = 0;
			}
			else
			{
				pml.velocity[2] += pm->s.gravity * pml.frameTime;
				if (pml.velocity[2] > 0)
					pml.velocity[2]  = 0;
			}
		}
		PM_StepSlideMove ();
	}
	else if ( pm->groundentity )
	{	// walking on ground
		pml.velocity[2] = 0; //!!! this is before the accel
		PM_Accelerate (wishdir, wishspeed, pm_accelerate);

// PGM	-- fix for negative trigger_gravity fields
//		pml.velocity[2] = 0;
		if(pm->s.gravity > 0)
			pml.velocity[2] = 0;
		else
			pml.velocity[2] -= pm->s.gravity * pml.frameTime;
// PGM

		if (!pml.velocity[0] && !pml.velocity[1])
			return;
		PM_StepSlideMove ();
	}
	else
	{	// not on ground, so little effect on velocity
		PM_AirAccelerate (wishdir, wishspeed, pm_airaccelerate);

		// add gravity
		pml.velocity[2] -= pm->s.gravity * pml.frameTime;
		PM_StepSlideMove ();
	}
}

/*
=============
PM_CheckWater

Sets pmove->waterlevel and pmove->watertype values.
=============
*/
static void PM_CheckWater()
{
	vec3_t	point;
	int		cont;

	// Pick a spot just above the players feet.
	point[0] = pml.origin[0] + ( pm->mins[0] + pm->maxs[0] ) * 0.5f;
	point[1] = pml.origin[1] + ( pm->mins[1] + pm->maxs[1] ) * 0.5f;
	point[2] = pml.origin[2] + pm->mins[2] + 1.0f;

	pm->waterlevel = WL_NONE;
	pm->watertype = CONTENTS_EMPTY;

	// check at feet level
	cont = pm->pointcontents( point );
	if ( cont & MASK_WATER ) {

		pm->watertype = cont;
		pm->waterlevel = WL_FEET;

		// check at waist level
		point[2] = pml.origin[2] + ( pm->mins[2] + pm->maxs[2] ) * 0.5f;
		cont = pm->pointcontents( point );
		if ( cont & MASK_WATER ) {

			pm->waterlevel = WL_WAIST;

			// check the eye position
			point[2] = pml.origin[2] + pm->viewheight;

			cont = pm->pointcontents( point );
			if ( cont & MASK_WATER ) {
				pm->waterlevel = WL_EYES;
			}
		}
	}
}

/*
=============
PM_CategorizePosition
=============
*/
static void PM_CategorizePosition( void )
{
	// if the player hull point one unit down is solid, the player
	// is on ground

	// see if standing on something solid

	// Doing this before we move may introduce a potential latency in water detection, but
	// doing it after can get us stuck on the bottom in water if the amount we move up
	// is less than the 1 pixel 'threshold' we're about to snap to.	Also, we'll call
	// this several times per frame, so we really need to avoid sticking to the bottom of
	// water on each call, and the converse case will correct itself if called twice.
	PM_CheckWater();

	// Spectators don't have a ground entity
	// or shooting up really fast.  Definitely not on ground
	if ( pml.ladder || pml.velocity[2] > NON_JUMP_VELOCITY )
	{
		memset( &pml.groundTrace, 0, sizeof( pml.groundTrace ) );
		pm->groundentity = NULL;
		return;
	}

	vec3_t point;

	point[0] = pml.origin[0];
	point[1] = pml.origin[1];
	point[2] = pml.origin[2] - 2.0f; // 0.25f

	bool moveToEndPos = false;

	// If our last groundentity exists, and we're not submerged
	if ( pm->groundentity && pm->waterlevel != WL_EYES )
	{
		// if walking and still think we're on ground, we'll extend trace down by stepsize so we don't bounce down slopes
		point[2] -= STEPSIZE;
		moveToEndPos = true;
	}

	pml.groundTrace = pm->trace( pml.origin, pm->mins, pm->maxs, point );

	if ( !pml.groundTrace.ent || ( pml.groundTrace.plane.normal[2] < MIN_STEP_NORMAL && !pml.groundTrace.startsolid ) )
	{
		pm->groundentity = NULL;
	}
	else
	{
		pm->groundentity = pml.groundTrace.ent;

		// hitting solid ground will end a waterjump
		if ( pm->s.pm_flags & PMF_TIME_WATERJUMP )
		{
			pm->s.pm_flags &= ~( PMF_TIME_WATERJUMP | PMF_TIME_TELEPORT );
			pm->s.pm_time = 0;
		}

		// If we hit a steep plane, we are not on ground
		if ( pml.groundTrace.plane.normal[2] < MIN_STEP_NORMAL )
		{
			pm->groundentity = NULL;	// too steep
		}
		else
		{
			pm->groundentity = pml.groundTrace.ent;	// Otherwise, point to index of ent under us.
		}

		if ( moveToEndPos && pml.groundTrace.fraction > 0.0f && pml.groundTrace.fraction < 1.0f )
		{
			VectorCopy( pml.groundTrace.endpos, pml.origin );
		}
	}

	if ( pm->numtouch < MAXTOUCH && pml.groundTrace.ent )
	{
		pm->touchents[pm->numtouch] = pml.groundTrace.ent;
		pm->numtouch++;
	}
}


/*
=============
PM_CheckJump
=============
*/
static void PM_CheckJump (void)
{
	if (pm->cmd.upmove < 10)
	{	// not holding jump
		pm->s.pm_flags &= ~PMF_JUMP_HELD;
		return;
	}

	// must wait for jump to be released
	if (pm->s.pm_flags & PMF_JUMP_HELD)
		return;

	if (pm->s.pm_type == PM_DEAD)
		return;

	if (pm->waterlevel >= 2)
	{	// swimming, not jumping
		pm->groundentity = NULL;

		if (pml.velocity[2] <= -300.0f)
			return;

		if (pm->watertype == CONTENTS_WATER)
			pml.velocity[2] = 100.0f;
		else if (pm->watertype == CONTENTS_SLIME)
			pml.velocity[2] = 80.0f;
		else
			pml.velocity[2] = 50.0f;

		// play swiming sound
		if ( pm->s.swim_time <= 0 )
		{
			// Don't play sound again for 1 second
			pm->s.swim_time = 1000;
			switch ( HackRandom( 0, 3 ) )
			{ 
			case 0:
				pm->playsound( "player/footsteps/wade1.wav", 1.0f );
				break;
			case 1:
				pm->playsound( "player/footsteps/wade2.wav", 1.0f );
				break;
			case 2:
				pm->playsound( "player/footsteps/wade3.wav", 1.0f );
				break;
			case 3:
				pm->playsound( "player/footsteps/wade4.wav", 1.0f );
				break;
			}
		}

		return;
	}

	if (pm->groundentity == NULL)
		return;		// in air, so no effect

	PM_PlayStepSound( SurfaceTypeFromFlags( pml.groundTrace.surface->flags ), 1.0f );

	pm->s.pm_flags |= PMF_JUMP_HELD;

	pm->groundentity = NULL;
	pml.velocity[2] = JUMP_VELOCITY;
}


/*
=============
PM_CheckSpecialMovement
=============
*/
static void PM_CheckSpecialMovement (void)
{
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;
	trace_t	trace;

	if (pm->s.pm_time)
		return;

	pml.ladder = false;

	// check for ladder
	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	VectorMA (pml.origin, 1, flatforward, spot);
	trace = pm->trace (pml.origin, pm->mins, pm->maxs, spot);
	if ((trace.fraction < 1) && (trace.contents & CONTENTS_LADDER))
		pml.ladder = true;

	// check for water jump
	if (pm->waterlevel != 2)
		return;

	VectorMA (pml.origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pm->pointcontents (spot);
	if (!(cont & CONTENTS_SOLID))
		return;

	spot[2] += 16;
	cont = pm->pointcontents (spot);
	if (cont)
		return;
	// jump out of water
	VectorScale (flatforward, 50, pml.velocity);
	pml.velocity[2] = 350;

	pm->s.pm_flags |= PMF_TIME_WATERJUMP;
	pm->s.pm_time = 255;
}

/*
===================
PM_SpectatorMove

SlartTodo: This does not work
===================
*/
static void PM_SpectatorMove()
{
	vec3_t		wishvel, wishdir;
	float		wishspeed;
	float		scale;
	int			i;

	// fly movement

	PM_Friction();

	// accelerate
	scale = PM_CmdScale( pm->cmd );

	if ( scale == 0.0f ) {
		VectorClear( wishvel );
	} else {
		for ( i = 0; i < 3; ++i ) {
			wishvel[i] = scale * ( pml.forward[i] * pm->cmd.forwardmove + pml.right[i] * pm->cmd.sidemove );
		}
	}

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );

	PM_Accelerate( wishdir, wishspeed, pm_flyaccelerate );

	PM_SlideMove();
}

/*
===================
PM_NoclipMove
===================
*/
static void PM_NoclipMove()
{
	int			i;
	float		speed, drop, friction, newspeed, stopspeed;
	float		scale, wishspeed;
	vec3_t		wishdir;

	pm->viewheight = STAND_VIEWHEIGHT;

	// friction
	speed = VectorLength( pml.velocity );
	if ( speed < 20.0f ) {
		VectorClear( pml.velocity );
	}
	else {
		stopspeed = pm_maxspeed * 0.3f;
		if ( speed < stopspeed ) {
			speed = stopspeed;
		}
		friction = pm_noclipfriction;
		drop = speed * friction * pml.frameTime;

		// scale the velocity
		newspeed = speed - drop;
		if ( newspeed < 0 ) {
			newspeed = 0;
		}

		VectorScale( pml.velocity, newspeed / speed, pml.velocity );
	}

	// accelerate
	scale = PM_CmdScale( pm->cmd );

	for ( i = 0; i < 3; ++i ) {
		wishdir[i] = pml.forward[i] * pm->cmd.forwardmove + pml.right[i] * pm->cmd.sidemove;
	}
	wishdir[2] += pm->cmd.upmove;

	wishspeed = VectorNormalize( wishdir ) * scale;

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	for ( i = 0; i < 3; ++i ) {
		pml.origin[i] += pml.frameTime * pml.velocity[i];
	}
}

/*
===================
PM_DeadMove
===================
*/
static void PM_DeadMove()
{
	float forward;

	if ( !pm->groundentity ) {
		return;
	}

	// extra friction
	forward = VectorLength( pml.velocity );
	forward -= 20.0f;
	if ( forward <= 0.0f ) {
		VectorClear( pml.velocity );
	}
	else {
		VectorNormalize( pml.velocity );
		VectorScale( pml.velocity, forward, pml.velocity );
	}
}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->viewheight
==============
*/
static void PM_CheckDuck()
{
	trace_t	trace;
	vec3_t end;

	// Start out as a 16x16 cube

	pm->mins[0] = -PLAYER_HULLSIZE;
	pm->mins[1] = -PLAYER_HULLSIZE;

	pm->maxs[0] = PLAYER_HULLSIZE;
	pm->maxs[1] = PLAYER_HULLSIZE;

	if ( pm->s.pm_type == PM_DEAD ) {
		pm->mins[2] = 0;
		pm->maxs[2] = GIB_HEIGHT;
		pm->viewheight = GIB_VIEWHEIGHT;
		return;
	}

	if ( pm->cmd.upmove < 0 && !pml.ladder ) {
		// duck
		pm->s.pm_flags |= PMF_DUCKED;
	}
	else if ( pm->s.pm_flags & PMF_DUCKED ) {
		// try to stand up

		end[0] = pml.origin[0];
		end[1] = pml.origin[1];
		end[2] = pml.origin[2] - ( STAND_HEIGHT - CROUCH_HEIGHT );

		trace = pm->trace( pml.origin, pm->mins, pm->maxs, end );
		if ( !trace.allsolid ) {
			pm->s.pm_flags &= ~PMF_DUCKED;
		}
	}

	if ( pm->s.pm_flags & PMF_DUCKED ) {
		pm->mins[2] = -CROUCH_HEIGHT;
		pm->maxs[2] = CROUCH_HEIGHT;
		pm->viewheight = CROUCH_VIEWHEIGHT;
	}
	else {
		pm->mins[2] = -STAND_HEIGHT;
		pm->maxs[2] = STAND_HEIGHT;
		pm->viewheight = STAND_VIEWHEIGHT;
	}
}

/*
===================
PM_GoodPosition
===================
*/
static bool PM_GoodPosition()
{
	trace_t trace;
	vec3_t start, end;

	if ( pm->s.pm_type == PM_SPECTATOR )
		return true;

	VectorCopy( pm->s.origin, start );
	VectorCopy( pm->s.origin, end );

	trace = pm->trace( start, pm->mins, pm->maxs, end );

	return !trace.allsolid;
}

/*
===================
PM_SnapPosition
===================
*/
static void PM_SnapPosition()
{
#if 0
	VectorCopy( pml.velocity, pm->s.velocity );
	VectorCopy( pml.origin, pm->s.origin );

	if ( PM_GoodPosition() )
		return;

	// go back to the last position
	VectorCopy( pml.previous_origin, pm->s.origin );
#endif

	VectorCopy( pml.velocity, pm->s.velocity );
	VectorCopy( pml.origin, pm->s.origin );
}

/*
===================
PM_ReduceTimers
===================
*/
static void PM_ReduceTimers()
{
	int msec;

//	msec = Clamp( pm->cmd.msec >> 3, 1, 200 );
	msec = pm->cmd.msec;

	// drop misc timing counter
	if ( pm->s.pm_time )
	{
		if ( msec >= pm->s.pm_time )
		{
			pm->s.pm_flags &= ~PMF_ALL_TIMES;
			pm->s.pm_time = 0;
		}
		else
		{
			pm->s.pm_time -= msec;
		}
	}

	if ( pm->s.time_step_sound > 0 )
	{
		pm->s.time_step_sound -= msec;
		if ( pm->s.time_step_sound < 0 )
		{
			pm->s.time_step_sound = 0;
		}
	}

	if ( pm->s.swim_time > 0 )
	{
		pm->s.swim_time -= msec;
		if ( pm->s.swim_time < 0 )
		{
			pm->s.swim_time = 0;
		}
	}
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated instead of a full move
================
*/
static void PM_UpdateViewAngles()
{
	int i;

	if ( pm->s.pm_flags & PMF_TIME_TELEPORT ) {
		pm->viewangles[YAW] = pm->cmd.angles[YAW] + pm->s.delta_angles[YAW];
		pm->viewangles[PITCH] = 0;
		pm->viewangles[ROLL] = 0;
	}
	else {
		// circularly clamp the angles with deltas
		for ( i = 0; i < 3; i++ ) {
			pm->viewangles[i] = pm->cmd.angles[i] + pm->s.delta_angles[i];
		}

		// Slart: This is clamped elsewhere
#if 0
		// don't let the player look up or down more than 90 degrees
		if ( pm->viewangles[PITCH] > 89 && pm->viewangles[PITCH] < 180 )
			pm->viewangles[PITCH] = 89;
		else if ( pm->viewangles[PITCH] < 271 && pm->viewangles[PITCH] >= 180 )
			pm->viewangles[PITCH] = 271;
#endif
	}
}

/*
===================
PM_Simulate
===================
*/
void PM_Simulate( pmove_t *pmove )
{
	pm = pmove;

	if ( pm->s.pm_type == PM_FREEZE ) {
		return;
	}

	// clear results
	pm->numtouch = 0;
	VectorClear( pm->viewangles );
	pm->viewheight = 0;
	pm->groundentity = nullptr;
	pm->watertype = 0;
	pm->waterlevel = 0;

	// clear our local vars
	memset( &pml, 0, sizeof( pml ) );

	// copy out the origin and velocity
	VectorCopy( pm->s.origin, pml.origin );
	VectorCopy( pm->s.velocity, pml.velocity );

	// save the old orgin in case we get stuck
	VectorCopy( pm->s.origin, pml.previousOrigin );

	// determine the time
	pml.frameTime = MS2SEC( pm->cmd.msec );

	// update our view angles
	PM_UpdateViewAngles();

	// view vectors
	AngleVectors( pm->viewangles, pml.forward, pml.right, pml.up );

	// special no clip mode
	if ( pm->s.pm_type == PM_NOCLIP ) {
		PM_NoclipMove();
		PM_SnapPosition();
		PM_ReduceTimers();
		return;
	}

	// set clip model size and viewheight
	PM_CheckDuck();

	// fly in spectator mode
	if ( pm->s.pm_type == PM_SPECTATOR ) {
		PM_SpectatorMove();
		PM_SnapPosition();
		PM_ReduceTimers();
		return;
	}

	// no control when dead
	if ( pm->s.pm_type == PM_DEAD || pm->s.pm_type == PM_GIB ) {
		pm->cmd.forwardmove = 0.0f;
		pm->cmd.sidemove = 0.0f;
		pm->cmd.upmove = 0.0f;
	}

	// set groundentity, watertype, and waterlevel
	PM_CategorizePosition();

	// check for ladder or waterjump
	PM_CheckSpecialMovement();

	// check for footstep sound
	PM_UpdateStepSound();

	// handle timers
	PM_ReduceTimers();

	// Move
	if ( pm->s.pm_type == PM_DEAD ) {
		PM_DeadMove();
	}
	else if ( pm->s.pm_flags & PMF_TIME_WATERJUMP ) {
		// jumping out of water
		pml.velocity[2] -= pm->s.gravity * pml.frameTime;
		if ( pml.velocity[2] < 0 )
		{	// cancel as soon as we are falling down again
			pm->s.pm_flags &= ~( PMF_TIME_WATERJUMP | PMF_TIME_TELEPORT );
			pm->s.pm_time = 0;
		}

		PM_StepSlideMove();
	}
	else {
		PM_CheckJump();

		PM_Friction();

		if ( pm->waterlevel >= 2 )
		{
			PM_WaterMove();
		}
		else
		{
			vec3_t	angles;

			VectorCopy( pm->viewangles, angles );
			if ( angles[PITCH] > 180 )
				angles[PITCH] = angles[PITCH] - 360;
			angles[PITCH] /= 3;

			AngleVectors( angles, pml.forward, pml.right, pml.up );

			PM_AirMove();
		}
	}

	// set groundentity, watertype, and waterlevel for final spot
	PM_CategorizePosition();

	PM_SnapPosition();
}
