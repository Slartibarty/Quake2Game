
#include "engine.h"

#include "pmove.h"

#define	STEPSIZE			16
#define GIB_VIEWHEIGHT		-8
#define CROUCH_VIEWHEIGHT	12
#define STAND_VIEWHEIGHT	28

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server

struct pml_t
{
	vec3_t		origin;			// full float precision
	vec3_t		velocity;		// full float precision

	vec3_t		forward, right, up;
	float		frametime;

	csurface_t	*groundsurface;
	cplane_t	groundplane;
	int			groundcontents;

	vec3_t		previous_origin;
	bool		ladder;
};

pmove_t		*pm;
pml_t		pml;


// movement parameters
static constexpr float	pm_stopspeed = 100;
static constexpr float	pm_maxspeed = 320;
static constexpr float	pm_duckspeed = 100;
static constexpr float	pm_accelerate = 10;
static constexpr float	pm_airaccelerate = 10;
static constexpr float	pm_wateraccelerate = 10;
static constexpr float	pm_friction = 6;
static constexpr float	pm_waterfriction = 1;
static constexpr float	pm_waterspeed = 400;

/*

  walking up a step should kill some velocity

*/


//-----------------------------------------------------------------------------

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

static void PM_PlayStepSound( int type, float fvol )
{
	int irand;

	pm->s.step_left = !pm->s.step_left;
	
	irand = HackRandom( 0, 1 ) + ( pm->s.step_left * 2 );

	// irand - 0,1 for right foot, 2,3 for left foot
	// used to alternate left and right foot
	// FIXME, move to player state

	// Not handled: Computer, flesh, glass
	switch (type)
	{
	default:
	case SURFTYPE_CONCRETE:
		switch (irand)
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
		switch(irand)
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
		switch(irand)
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
		switch(irand)
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
		switch(irand)
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
	}
}	

void PM_UpdateStepSound( void )
{
	int	fWalking;
	float fvol;
	vec3_t knee;
	vec3_t feet;
	vec3_t center;
	float height;
	float speed;
	float velrun;
	float velwalk;
	float flduck;
	int step;

	if ( pm->s.time_step_sound > 0 )
		return;

	assert( pm->s.pm_type != PM_FREEZE );

	speed = VectorLength( pml.velocity );

	// UNDONE: need defined numbers for run, walk, crouch, crouch run velocities!!!!	
	if ( pm->s.pm_flags & PMF_DUCKED )
	{
		velwalk = 60;		// These constants should be based on cl_movespeedkey * cl_forwardspeed somehow
		velrun = 80;		// UNDONE: Move walking to server
		flduck = 100;
	}
	else
	{
		velwalk = 120;
		velrun = 210;
		flduck = 0;
	}

	// If we're on a ladder or on the ground, and we're moving fast enough,
	//  play step sound.  Also, if pmove->flTimeStepSound is zero, get the new
	//  sound right away - we just started moving in new level.
	if ( pm->groundentity &&
		( speed > 0.0f ) &&
		( speed >= velwalk || !pm->s.time_step_sound ) )
	{
		fWalking = speed < velrun;

		VectorCopy( pml.origin, center );
		VectorCopy( pml.origin, knee );
		VectorCopy( pml.origin, feet );

		height = 72;

		knee[2] = pml.origin[2] - 0.3f * height;
		feet[2] = pml.origin[2] - 0.5f * height;

		// find out what we're stepping in or on...
		{
			// find texture under player, if different from current texture, 
			// get material type
			assert( pml.groundsurface );
			step = SurfaceTypeFromFlags( pml.groundsurface->flags );

			switch ( step )
			{
			default:
			case SURFTYPE_CONCRETE:
				fvol = fWalking ? 0.2f : 0.5f;
				pm->s.time_step_sound = fWalking ? 400 : 300;
				break;

			case SURFTYPE_METAL:
				fvol = fWalking ? 0.2f : 0.5f;
				pm->s.time_step_sound = fWalking ? 400 : 300;
				break;

			case SURFTYPE_VENT:
				fvol = fWalking ? 0.4f : 0.7f;
				pm->s.time_step_sound = fWalking ? 400 : 300;
				break;

			case SURFTYPE_DIRT:
			case SURFTYPE_GRASS:
			case SURFTYPE_SAND:
			case SURFTYPE_ROCK:
				fvol = fWalking ? 0.25f : 0.55f;
				pm->s.time_step_sound = fWalking ? 400 : 300;
				break;

			case SURFTYPE_WOOD:
				fvol = fWalking ? 0.25f : 0.55f;
				pm->s.time_step_sound = fWalking ? 400 : 300;
				break;

			case SURFTYPE_TILE:
			case SURFTYPE_COMPUTER:
			case SURFTYPE_GLASS:
			case SURFTYPE_FLESH:
				fvol = fWalking ? 0.2f : 0.5f;
				pm->s.time_step_sound = fWalking ? 400 : 300;
				break;
			}
		}

		pm->s.time_step_sound += flduck; // slower step time if ducking

		// play the sound
		// 35% volume if ducking
		if ( pm->s.pm_flags & PMF_DUCKED )
		{
			fvol *= 0.35f;
		}

		PM_PlayStepSound( step, fvol );
	}
}

//-----------------------------------------------------------------------------


/*
==================
PM_ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define	STOP_EPSILON	0.1f

void PM_ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff;
	float	change;
	int		i;
	
	backoff = DotProduct (in, normal) * overbounce;

	for (i=0 ; i<3 ; i++)
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}
}


/*
==================
PM_StepSlideMove

Each intersection will try to step over the obstruction instead of
sliding along it.

Returns a new origin, velocity, and contact entity
Does not modify any world state?
==================
*/
#define	MIN_STEP_NORMAL	0.7f	// can't step up onto very steep slopes
#define	MAX_CLIP_PLANES	5
void PM_StepSlideMove_ (void)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity, original_velocity;
	vec3_t      new_velocity;
	int			i, j;
	trace_t	trace;
	vec3_t		end;
	float		time_left, allFraction;
	
	numbumps = 4; // Bump up to four times
	
	numplanes = 0; // Assume not sliding along any planes
	VectorCopy (pml.velocity, original_velocity);
	VectorCopy (pml.velocity, primal_velocity);
	
	allFraction = 0;
	time_left = pml.frametime; // Total time for this movement operation

	for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
	{
		if ( !pml.velocity[0] && !pml.velocity[1] && !pml.velocity[2] )
			break;

		// Assume we can move all the way from the current origin to the
		//  end point.
		for (i=0 ; i<3 ; i++)
			end[i] = pml.origin[i] + time_left * pml.velocity[i];

		trace = pm->trace (pml.origin, pm->mins, pm->maxs, end);

		allFraction += trace.fraction;
		// If we started in a solid object, or we were in solid space
		//  the whole way, zero out our velocity and return that we
		//  are blocked by floor and wall.
		if (trace.allsolid)
		{	// entity is trapped in another solid
			//pml.velocity[2] = 0;	// don't build up falling damage
			VectorClear (pml.velocity); // Zero out all velocity
			return;
		}

		// If we moved some portion of the total distance, then
		//  copy the end position into the pmove->origin and 
		//  zero the plane counter.
		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy (trace.endpos, pml.origin);
			VectorCopy (pml.velocity, original_velocity);
			numplanes = 0;
		}

		// If we covered the entire distance, we are done
		//  and can return.
		if (trace.fraction == 1)
			 break;		// moved the entire distance

		//if (!trace.ent)
		//	Sys_Error ("PM_PlayerTrace: !trace.ent");

		// Save entity that blocked us (since fraction was < 1.0)
		//  for contact
		// Add it if it's not already in the list!!!
		if (pm->numtouch < MAXTOUCH && trace.ent)
		{
			pm->touchents[pm->numtouch] = trace.ent;
			pm->numtouch++;
		} // SlartTodo: This block differs
		
		// Reduce amount of pmove->frametime left by total time left * fraction
		//  that we covered.
		time_left -= time_left * trace.fraction;

		// Did we run out of planes to clip against?
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			//  Stop our movement if so.
			VectorClear (pml.velocity);
			assert (0);
			break;
		}

		// Set up next clipping plane
		VectorCopy (trace.plane.normal, planes[numplanes]);
		numplanes++;

//
// modify original_velocity so it parallels all of the clip planes
//
		if ( (!pm->groundentity) || (pm_friction != 1) )	// relfect player velocity
		{
			for ( i = 0; i < numplanes; i++ )
			{
				if ( planes[i][2] > 0.7f  )
				{// floor or slope
					PM_ClipVelocity( original_velocity, planes[i], new_velocity, 1 );
					VectorCopy( new_velocity, original_velocity );
				}
				else
					PM_ClipVelocity( original_velocity, planes[i], new_velocity, 1.0 + /*pmove->movevars->bounce*/ 0 * (1-pm_friction) );
			}

			VectorCopy( new_velocity, pml.velocity );
			VectorCopy( new_velocity, original_velocity );
		}
		else
		{
			for (i=0 ; i<numplanes ; i++)
			{
				PM_ClipVelocity (
					original_velocity,
					planes[i],
					pml.velocity,
					1);
				for (j=0 ; j<numplanes ; j++)
					if (j != i)
					{
						// Are we now moving against this plane?
						if (DotProduct (pml.velocity, planes[j]) < 0)
							break;	// not ok
					}
				if (j == numplanes)  // Didn't have to clip, so we're ok
					break;
			}
			
			// Did we go all the way through plane set
			if (i != numplanes)
			{	// go along this plane
				// pmove->velocity is set in clipping call, no need to set again.
				;  
			}
			else
			{	// go along the crease
				if (numplanes != 2)
				{
					//Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
					VectorClear( pml.velocity );
					//Con_DPrintf("Trapped 4\n");

					break;
				}
				CrossProduct (planes[0], planes[1], dir);
				d = DotProduct (dir, pml.velocity);
				VectorScale (dir, d, pml.velocity );
			}

	//
	// if original velocity is against the original velocity, stop dead
	// to avoid tiny occilations in sloping corners
	//
			if (DotProduct (pml.velocity, primal_velocity) <= 0)
			{
				//Con_DPrintf("Back\n");
				VectorClear( pml.velocity );
				break;
			}
		}
	}

	if ( allFraction == 0 )
	{
		VectorClear (pml.velocity);
		//Con_DPrintf( "Don't stick\n" );
	}

	/*if (pm->s.pm_time)
	{
		VectorCopy (primal_velocity, pml.velocity);
	}*/
}

/*
==================
PM_StepSlideMove

==================
*/
void PM_StepSlideMove (void)
{
	vec3_t		start_o, start_v;
	vec3_t		down_o, down_v;
	trace_t		trace;
	float		down_dist, up_dist;
//	vec3_t		delta;
	vec3_t		up, down;

	VectorCopy (pml.origin, start_o);
	VectorCopy (pml.velocity, start_v);

	PM_StepSlideMove_ ();

	VectorCopy (pml.origin, down_o);
	VectorCopy (pml.velocity, down_v);

	VectorCopy (start_o, up);
	up[2] += STEPSIZE;

	trace = pm->trace (up, pm->mins, pm->maxs, up);
	if (trace.allsolid)
		return;		// can't step up

	// try sliding above
	VectorCopy (up, pml.origin);
	VectorCopy (start_v, pml.velocity);

	PM_StepSlideMove_ ();

	// push down the final amount
	VectorCopy (pml.origin, down);
	down[2] -= STEPSIZE;
	trace = pm->trace (pml.origin, pm->mins, pm->maxs, down);
	if (!trace.allsolid)
	{
		VectorCopy (trace.endpos, pml.origin);
	}

#if 0
	VectorSubtract (pml.origin, up, delta);
	up_dist = DotProduct (delta, start_v);

	VectorSubtract (down_o, start_o, delta);
	down_dist = DotProduct (delta, start_v);
#else
	VectorCopy(pml.origin, up);

	// decide which one went farther
	down_dist = (down_o[0] - start_o[0])*(down_o[0] - start_o[0])
		+ (down_o[1] - start_o[1])*(down_o[1] - start_o[1]);
	up_dist = (up[0] - start_o[0])*(up[0] - start_o[0])
		+ (up[1] - start_o[1])*(up[1] - start_o[1]);
#endif

	if (down_dist > up_dist || trace.plane.normal[2] < MIN_STEP_NORMAL)
	{
		VectorCopy (down_o, pml.origin);
		VectorCopy (down_v, pml.velocity);
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
void PM_Friction (void)
{
	float	*vel;
	float	speed, newspeed, control;
	float	friction;
	float	drop;
	
	// Get velocity
	vel = pml.velocity;
	
	// Calculate speed
	speed = VectorLength( vel );

	// If too slow, return
	if (speed < 0.1f)
	{
		vel[0] = 0;
		vel[1] = 0;
		return;
	}

	drop = 0;

// apply ground friction
	if ((pm->groundentity && pml.groundsurface && !(pml.groundsurface->flags & SURF_SLICK) ) || (pml.ladder) )
	{
		friction = pm_friction;
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*pml.frametime;
	}

// apply water friction
	if (pm->waterlevel && !pml.ladder)
		drop += speed*pm_waterfriction*pm->waterlevel*pml.frametime;

// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
	{
		newspeed = 0;
	}

	// Determine proportion of old speed we are using.
	newspeed /= speed;

	// Adjust velocity according to proportion.
	vel[0] *= newspeed;
	vel[1] *= newspeed;
	vel[2] *= newspeed;
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
void PM_Accelerate (vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed;

	// See if we are changing direction a bi
	currentspeed = DotProduct (pml.velocity, wishdir);

	// Reduce wishspeed by the amount of veer.
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done.
	if (addspeed <= 0)
		return;

	// Determine amount of accleration.
	accelspeed = accel*pml.frametime*wishspeed;

	// Cap at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	// Adjust velocity.
	for (i=0 ; i<3 ; i++)
		pml.velocity[i] += accelspeed*wishdir[i];	
}

void PM_AirAccelerate (vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed, wishspd = wishspeed;
		
	// Cap speed
	if (wishspd > 30)
		wishspd = 30;
	// Determine veer amount
	currentspeed = DotProduct (pml.velocity, wishdir);
	// See how much to add
	addspeed = wishspd - currentspeed;
	// If not adding any, done.
	if (addspeed <= 0)
		return;

	// Determine acceleration speed after acceleration
	accelspeed = accel * wishspeed * pml.frametime;
	// Cap it
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	// Adjust pmove vel.
	for (i=0 ; i<3 ; i++)
		pml.velocity[i] += accelspeed*wishdir[i];	
}

/*
=============
PM_AddCurrents
=============
*/
void PM_AddCurrents (vec3_t	wishvel)
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

		if (pml.groundcontents & CONTENTS_CURRENT_0)
			v[0] += 1;
		if (pml.groundcontents & CONTENTS_CURRENT_90)
			v[1] += 1;
		if (pml.groundcontents & CONTENTS_CURRENT_180)
			v[0] -= 1;
		if (pml.groundcontents & CONTENTS_CURRENT_270)
			v[1] -= 1;
		if (pml.groundcontents & CONTENTS_CURRENT_UP)
			v[2] += 1;
		if (pml.groundcontents & CONTENTS_CURRENT_DOWN)
			v[2] -= 1;

		VectorMA (wishvel, 100 /* pm->groundentity->speed */, v, wishvel);
	}
}


/*
===================
PM_WaterMove

===================
*/
void PM_WaterMove (void)
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
void PM_AirMove (void)
{
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		maxspeed;

	// Copy movement amounts
	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.sidemove;
	
//!!!!! pitch should be 1/3 so this isn't needed??!
#if 1
	// Zero out z components of movement vectors
	pml.forward[2] = 0;
	pml.right[2] = 0;
	// Renormalize
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);
#endif

	// Determine x and y parts of velocity
	for (i=0 ; i<2 ; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	// Zero out z part of velocity
	wishvel[2] = 0;

	// Add currents
	PM_AddCurrents (wishvel);

	// Determine maginitude of speed of move
	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

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
				pml.velocity[2] -= pm->s.gravity * pml.frametime;
				if (pml.velocity[2] < 0)
					pml.velocity[2]  = 0;
			}
			else
			{
				pml.velocity[2] += pm->s.gravity * pml.frametime;
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
			pml.velocity[2] -= pm->s.gravity * pml.frametime;
// PGM

		if (!pml.velocity[0] && !pml.velocity[1])
			return;
		PM_StepSlideMove ();
	}
	else
	{	// not on ground, so little effect on velocity
		PM_AirAccelerate (wishdir, wishspeed, pm_airaccelerate);

		// add gravity
		pml.velocity[2] -= pm->s.gravity * pml.frametime;
		PM_StepSlideMove ();
	}
}



/*
=============
PM_CatagorizePosition
=============
*/
void PM_CatagorizePosition (void)
{
	vec3_t		point;
	int			cont;
	trace_t		trace;
	int			sample1;
	int			sample2;

// if the player hull point one unit down is solid, the player
// is on ground

// see if standing on something solid	
	point[0] = pml.origin[0];
	point[1] = pml.origin[1];
	point[2] = pml.origin[2] - 0.25f;
	if (pml.velocity[2] > 180.0f)	// Shooting up really fast.  Definitely not on ground.
	{
		pm->s.pm_flags &= ~PMF_ON_GROUND;
		pm->groundentity = NULL;
	}
	else
	{
		trace = pm->trace (pml.origin, pm->mins, pm->maxs, point);
		pml.groundplane = trace.plane;
		pml.groundsurface = trace.surface;
		pml.groundcontents = trace.contents;

		if (!trace.ent || (trace.plane.normal[2] < 0.7f && !trace.startsolid) )
		{
			pm->groundentity = NULL;
			pm->s.pm_flags &= ~PMF_ON_GROUND;
		}
		else
		{
			pm->groundentity = trace.ent;

			// hitting solid ground will end a waterjump
			if (pm->s.pm_flags & PMF_TIME_WATERJUMP)
			{
				pm->s.pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
				pm->s.pm_time = 0;
			}

			if (! (pm->s.pm_flags & PMF_ON_GROUND) )
			{	// just hit the ground
				pm->s.pm_flags |= PMF_ON_GROUND;
				// don't do landing time if we were just going down a slope
				if (pml.velocity[2] < -200.0f)
				{
					pm->s.pm_flags |= PMF_TIME_LAND;
					// don't allow another jump for a little while
					if (pml.velocity[2] < -400.0f)
						pm->s.pm_time = 25;	
					else
						pm->s.pm_time = 18;
				}
			}
		}

#if 0
		if (trace.fraction < 1.0 && trace.ent && pml.velocity[2] < 0)
			pml.velocity[2] = 0;
#endif

		if (pm->numtouch < MAXTOUCH && trace.ent)
		{
			pm->touchents[pm->numtouch] = trace.ent;
			pm->numtouch++;
		}
	}

//
// get waterlevel, accounting for ducking
//
	pm->waterlevel = 0;
	pm->watertype = 0;

	sample2 = pm->viewheight - pm->mins[2];
	sample1 = sample2 / 2;

	point[2] = pml.origin[2] + pm->mins[2] + 1.0f;
	cont = pm->pointcontents (point);

	if (cont & MASK_WATER)
	{
		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pml.origin[2] + pm->mins[2] + sample1;
		cont = pm->pointcontents (point);
		if (cont & MASK_WATER)
		{
			pm->waterlevel = 2;
			point[2] = pml.origin[2] + pm->mins[2] + sample2;
			cont = pm->pointcontents (point);
			if (cont & MASK_WATER)
				pm->waterlevel = 3;
		}
	}

}


/*
=============
PM_CheckJump
=============
*/
void PM_CheckJump (void)
{
	if (pm->s.pm_flags & PMF_TIME_LAND)
	{	// hasn't been long enough since landing to jump again
		return;
	}

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
		return;
	}

	if (pm->groundentity == NULL)
		return;		// in air, so no effect

	PM_PlayStepSound( SurfaceTypeFromFlags( pml.groundsurface->flags ), 1.0f );

	pm->s.pm_flags |= PMF_JUMP_HELD;

	pm->groundentity = NULL;
	pml.velocity[2] += 270.0f;
	if (pml.velocity[2] < 270.0f)
		pml.velocity[2] = 270.0f;
}


/*
=============
PM_CheckSpecialMovement
=============
*/
void PM_CheckSpecialMovement (void)
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
===============
PM_FlyMove
===============
*/
void PM_FlyMove (qboolean doclip)
{
	float	speed, drop, friction, control, newspeed;
	float	currentspeed, addspeed, accelspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	vec3_t		end;
	trace_t	trace;

	pm->viewheight = STAND_VIEWHEIGHT;

	// friction

	speed = VectorLength (pml.velocity);
	if (speed < 1)
	{
		VectorCopy (vec3_origin, pml.velocity);
	}
	else
	{
		drop = 0;

		friction = pm_friction*1.5f;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (pml.velocity, newspeed, pml.velocity);
	}

	// accelerate
	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.sidemove;
	
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for (i=0 ; i<3 ; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] += pm->cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if (wishspeed > pm_maxspeed)
	{
		VectorScale (wishvel, pm_maxspeed/wishspeed, wishvel);
		wishspeed = pm_maxspeed;
	}


	currentspeed = DotProduct(pml.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = pm_accelerate*pml.frametime*wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	for (i=0 ; i<3 ; i++)
		pml.velocity[i] += accelspeed*wishdir[i];	

	if (doclip) {
		for (i=0 ; i<3 ; i++)
			end[i] = pml.origin[i] + pml.frametime * pml.velocity[i];

		trace = pm->trace (pml.origin, pm->mins, pm->maxs, end);

		VectorCopy (trace.endpos, pml.origin);
	} else {
		// move
		VectorMA (pml.origin, pml.frametime, pml.velocity, pml.origin);
	}
}


/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->viewheight
==============
*/
void PM_CheckDuck (void)
{
	trace_t	trace;

	pm->mins[0] = -16;
	pm->mins[1] = -16;

	pm->maxs[0] = 16;
	pm->maxs[1] = 16;

	if (pm->s.pm_type == PM_GIB)
	{
		pm->mins[2] = 0;
		pm->maxs[2] = 16;
		pm->viewheight = GIB_VIEWHEIGHT;
		return;
	}

	pm->mins[2] = -24;

	if (pm->s.pm_type == PM_DEAD)
	{
		pm->s.pm_flags |= PMF_DUCKED;
	}
	else if (pm->cmd.upmove < 0 && (pm->s.pm_flags & PMF_ON_GROUND) )
	{	// duck
		pm->s.pm_flags |= PMF_DUCKED;
	}
	else
	{	// stand up if possible
		if (pm->s.pm_flags & PMF_DUCKED)
		{
			// try to stand up
			pm->maxs[2] = 32;
			trace = pm->trace (pml.origin, pm->mins, pm->maxs, pml.origin);
			if (!trace.allsolid)
				pm->s.pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pm->s.pm_flags & PMF_DUCKED)
	{
		pm->maxs[2] = 4;
		pm->viewheight = CROUCH_VIEWHEIGHT;
	}
	else
	{
		pm->maxs[2] = 32;
		pm->viewheight = STAND_VIEWHEIGHT;
	}
}


/*
==============
PM_DeadMove
==============
*/
void PM_DeadMove (void)
{
	float	forward;

	if (!pm->groundentity)
		return;

	// extra friction

	forward = VectorLength (pml.velocity);
	forward -= 20;
	if (forward <= 0)
	{
		VectorClear (pml.velocity);
	}
	else
	{
		VectorNormalize (pml.velocity);
		VectorScale (pml.velocity, forward, pml.velocity);
	}
}


qboolean	PM_GoodPosition (void)
{
	trace_t	trace;
	vec3_t	origin, end;
	int		i;

	if (pm->s.pm_type == PM_SPECTATOR)
		return true;

	for (i=0 ; i<3 ; i++)
		origin[i] = end[i] = pm->s.origin[i]*0.125f;
	trace = pm->trace (origin, pm->mins, pm->maxs, end);

	return !trace.allsolid;
}

/*
================
PM_SnapPosition

On exit, the origin will have a value that is pre-quantized to the 0.125
precision of the network channel and in a valid position.
================
*/
void PM_SnapPosition (void)
{
	int		sign[3];
	int		i, j, bits;
	short	base[3];
	// try all single bits first
	static int jitterbits[8] = {0,4,1,2,3,5,6,7};

	// snap velocity to eigths
	for (i=0 ; i<3 ; i++)
		pm->s.velocity[i] = (int)(pml.velocity[i]*8);

	for (i=0 ; i<3 ; i++)
	{
		if (pml.origin[i] >= 0)
			sign[i] = 1;
		else 
			sign[i] = -1;
		pm->s.origin[i] = (int)(pml.origin[i]*8);
		if (pm->s.origin[i]*0.125f == pml.origin[i])
			sign[i] = 0;
	}
	_VectorCopy (pm->s.origin, base);

	// try all combinations
	for (j=0 ; j<8 ; j++)
	{
		bits = jitterbits[j];
		_VectorCopy (base, pm->s.origin);
		for (i=0 ; i<3 ; i++)
			if (bits & (1<<i) )
				pm->s.origin[i] += sign[i];

		if (PM_GoodPosition ())
			return;
	}

	// go back to the last position
	_VectorCopy (pml.previous_origin, pm->s.origin);
//	Com_DPrintf ("using previous_origin\n");
}

#if 0
//NO LONGER USED
/*
================
PM_InitialSnapPosition

================
*/
void PM_InitialSnapPosition (void)
{
	int		x, y, z;
	short	base[3];

	VectorCopy (pm->s.origin, base);

	for (z=1 ; z>=-1 ; z--)
	{
		pm->s.origin[2] = base[2] + z;
		for (y=1 ; y>=-1 ; y--)
		{
			pm->s.origin[1] = base[1] + y;
			for (x=1 ; x>=-1 ; x--)
			{
				pm->s.origin[0] = base[0] + x;
				if (PM_GoodPosition ())
				{
					pml.origin[0] = pm->s.origin[0]*0.125;
					pml.origin[1] = pm->s.origin[1]*0.125;
					pml.origin[2] = pm->s.origin[2]*0.125;
					VectorCopy (pm->s.origin, pml.previous_origin);
					return;
				}
			}
		}
	}

	Com_DPrintf ("Bad InitialSnapPosition\n");
}
#else
/*
================
PM_InitialSnapPosition

================
*/
void PM_InitialSnapPosition(void)
{
	int        x, y, z;
	short      base[3];
	static int offset[3] = { 0, -1, 1 };

	_VectorCopy (pm->s.origin, base);

	for ( z = 0; z < 3; z++ ) {
		pm->s.origin[2] = base[2] + offset[ z ];
		for ( y = 0; y < 3; y++ ) {
			pm->s.origin[1] = base[1] + offset[ y ];
			for ( x = 0; x < 3; x++ ) {
				pm->s.origin[0] = base[0] + offset[ x ];
				if (PM_GoodPosition ()) {
					pml.origin[0] = pm->s.origin[0]*0.125f;
					pml.origin[1] = pm->s.origin[1]*0.125f;
					pml.origin[2] = pm->s.origin[2]*0.125f;
					_VectorCopy (pm->s.origin, pml.previous_origin);
					return;
				}
			}
		}
	}

	Com_DPrintf ("Bad InitialSnapPosition\n");
}

#endif

/*
================
PM_ClampAngles

================
*/
void PM_ClampAngles (void)
{
	short	temp;
	int		i;

	if (pm->s.pm_flags & PMF_TIME_TELEPORT)
	{
		pm->viewangles[YAW] = SHORT2ANGLE(pm->cmd.angles[YAW] + pm->s.delta_angles[YAW]);
		pm->viewangles[PITCH] = 0;
		pm->viewangles[ROLL] = 0;
	}
	else
	{
		// circularly clamp the angles with deltas
		for (i=0 ; i<3 ; i++)
		{
			temp = pm->cmd.angles[i] + pm->s.delta_angles[i];
			pm->viewangles[i] = SHORT2ANGLE(temp);
		}

		// don't let the player look up or down more than 90 degrees
		if (pm->viewangles[PITCH] > 89 && pm->viewangles[PITCH] < 180)
			pm->viewangles[PITCH] = 89;
		else if (pm->viewangles[PITCH] < 271 && pm->viewangles[PITCH] >= 180)
			pm->viewangles[PITCH] = 271;
	}
	AngleVectors (pm->viewangles, pml.forward, pml.right, pml.up);
}

static void PM_ReduceTimers()
{
	if ( pm->s.time_step_sound > 0 )
	{
		pm->s.time_step_sound -= pm->cmd.msec;
		if ( pm->s.time_step_sound < 0 )
		{
			pm->s.time_step_sound = 0;
		}
	}
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove (pmove_t *pmove)
{
	pm = pmove;

	// clear results
	pm->numtouch = 0;
	VectorClear (pm->viewangles);
	pm->viewheight = 0;
	pm->groundentity = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	// clear all pmove local vars
	memset (&pml, 0, sizeof(pml));

	// convert origin and velocity to float values
	pml.origin[0] = pm->s.origin[0]*0.125f;
	pml.origin[1] = pm->s.origin[1]*0.125f;
	pml.origin[2] = pm->s.origin[2]*0.125f;

	pml.velocity[0] = pm->s.velocity[0]*0.125f;
	pml.velocity[1] = pm->s.velocity[1]*0.125f;
	pml.velocity[2] = pm->s.velocity[2]*0.125f;

	// save old org in case we get stuck
	_VectorCopy (pm->s.origin, pml.previous_origin);

	pml.frametime = pm->cmd.msec * mathconst::MillisecondsToSeconds;

	PM_ReduceTimers();

	PM_ClampAngles ();

	if (pm->s.pm_type == PM_SPECTATOR)
	{
		PM_FlyMove (false);
		PM_SnapPosition ();
		return;
	}

	if (pm->s.pm_type >= PM_DEAD)
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.sidemove = 0;
		pm->cmd.upmove = 0;
	}

	if (pm->s.pm_type == PM_FREEZE)
		return;		// no movement at all

	// set mins, maxs, and viewheight
	PM_CheckDuck ();

	if (pm->snapinitial)
		PM_InitialSnapPosition ();

	// set groundentity, watertype, and waterlevel
	PM_CatagorizePosition ();

	PM_UpdateStepSound();

	if (pm->s.pm_type == PM_DEAD)
		PM_DeadMove ();

	PM_CheckSpecialMovement ();

	// drop timing counter
	if (pm->s.pm_time)
	{
		int		msec;

		msec = pm->cmd.msec >> 3;
		if (!msec)
			msec = 1;
		if ( msec >= pm->s.pm_time) 
		{
			pm->s.pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
			pm->s.pm_time = 0;
		}
		else
			pm->s.pm_time -= msec;
	}

	if (pm->s.pm_flags & PMF_TIME_TELEPORT)
	{	// teleport pause stays exactly in place
	}
	else if (pm->s.pm_flags & PMF_TIME_WATERJUMP)
	{	// waterjump has no control, but falls
		pml.velocity[2] -= pm->s.gravity * pml.frametime;
		if (pml.velocity[2] < 0)
		{	// cancel as soon as we are falling down again
			pm->s.pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
			pm->s.pm_time = 0;
		}

		PM_StepSlideMove ();
	}
	else
	{
		PM_CheckJump ();

		PM_Friction ();

		if (pm->waterlevel >= 2)
			PM_WaterMove ();
		else {
			vec3_t	angles;

			VectorCopy(pm->viewangles, angles);
			if (angles[PITCH] > 180)
				angles[PITCH] = angles[PITCH] - 360;
			angles[PITCH] /= 3;

			AngleVectors (angles, pml.forward, pml.right, pml.up);

			PM_AirMove ();
		}
	}

	// set groundentity, watertype, and waterlevel for final spot
	PM_CatagorizePosition ();

	PM_SnapPosition ();
}

