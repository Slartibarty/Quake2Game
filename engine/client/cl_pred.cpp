/*
===================================================================================================

	Prediction

===================================================================================================
*/

#include "cl_local.h"

/*
========================
CL_CheckPredictionError
========================
*/
void CL_CheckPredictionError()
{
	int		frame;
	vec3_t	delta;
	float	len;

	if (!cl_predict.GetBool() || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
		return;

	// calculate the last usercmd_t we sent that the server has processed
	frame = cls.netchan.incoming_acknowledged;
	frame &= (CMD_BACKUP-1);

	// compare what the server returned with what we had predicted it to be
	VectorSubtract (cl.frame.playerstate.pmove.origin, cl.predicted_origins[frame], delta);

	// save the prediction error for interpolation
	len = fabs(delta[0]) + fabs(delta[1]) + fabs(delta[2]);
	if (len > 80.0f)
	{	// a teleport or something
		VectorClear (cl.predError);
	}
	else
	{
		if (cl_showmiss.GetBool() && (delta[0] || delta[1] || delta[2]) )
			Com_Printf ("prediction miss on %i: %f\n", cl.frame.serverframe, 
			delta[0] + delta[1] + delta[2]);

		VectorCopy (cl.frame.playerstate.pmove.origin, cl.predicted_origins[frame]);

		// save for error interpolation
		VectorCopy( delta, cl.predError );
	}
}

//=================================================================================================

enum solid_t
{
	SOLID_NOT,			// no interaction with other objects
	SOLID_TRIGGER,		// only touch when inside, after moving
	SOLID_BBOX,			// touch on edge
	SOLID_BSP,			// bsp clip, touch on edge
	SOLID_PHYSICS		// rigid body
};

/*
========================
CL_ClipMoveToEntities
========================
*/
static void CL_ClipMoveToEntities ( vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, trace_t *tr )
{
	int				i, x, zd, zu;
	trace_t			trace;
	int				headnode;
	float *			angles;
	entityState_t *	ent;
	int				num;
	cmodel_t *		cmodel;
	vec3_t			bmins, bmaxs;

	for (i=0 ; i<cl.frame.num_entities ; i++)
	{
		num = (cl.frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_parse_entities[num];

		if (!ent->solid)
			continue;

		if (ent->number == cl.playernum+1)
			continue;

		if (ent->solid == 31)
		{
			// special value for bmodel
			cmodel = cl.model_clip[ent->modelindex];
			if (!cmodel)
				continue;
			headnode = cmodel->headnode;
			angles = ent->angles;
		}
		else
		{
			// encoded bbox
			x = 8*(ent->solid & 31);
			zd = 8*((ent->solid>>5) & 31);
			zu = 8*((ent->solid>>10) & 63) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			headnode = CM_HeadnodeForBox (bmins, bmaxs);
			angles = vec3_origin;	// boxes don't rotate
		}

		if (tr->allsolid)
			return;

#if 0
		if ( ent->solid == SOLID_PHYSICS )
		{
			Physics::RayCast rayCast( start, end, mins, maxs );
			Physics::ClientPlayerTrace( rayCast, ent->origin, ent->angles, trace );
		}
		else
#endif
		{
			CM_TransformedBoxTrace( start, end,
				mins, maxs, headnode, MASK_PLAYERSOLID,
				ent->origin, angles, trace );
		}

		if ( trace.allsolid || trace.startsolid || trace.fraction < tr->fraction )
		{
			trace.ent = (edict_t *)ent;
			if ( tr->startsolid )
			{
				*tr = trace;
				tr->startsolid = true;
			}
			else
			{
				*tr = trace;
			}
		}
		else if ( trace.startsolid )
		{
			tr->startsolid = true;
		}
	}
}

static trace_t CL_PMTrace( vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end )
{
	trace_t	t;

	// check against world
	t = CM_BoxTrace( start, end, mins, maxs, 0, MASK_PLAYERSOLID );
	if ( t.fraction < 1.0f )
	{
		t.ent = (edict_t *)1;
	}

	// check all other solid models
	CL_ClipMoveToEntities( start, mins, maxs, end, &t );

	return t;
}

static int CL_PMPointContents( vec3_t point )
{
	int num;
	entity_state_t *ent;
	cmodel_t *cmodel;

	int contents = CM_PointContents( point, 0 );

	for ( int i = 0; i < cl.frame.num_entities; i++ )
	{
		num = ( cl.frame.parse_entities + i ) & ( MAX_PARSE_ENTITIES - 1 );
		ent = &cl_parse_entities[num];

		if ( ent->solid != 31 )
		{
			// special value for bmodel
			continue;
		}

		cmodel = cl.model_clip[ent->modelindex];
		if ( !cmodel )
		{
			continue;
		}

		contents |= CM_TransformedPointContents( point, cmodel->headnode, ent->origin, ent->angles );
	}

	return contents;
}

static void CL_PMPlaySound( const char *sample, float volume )
{
}

/*
========================
CL_PredictMovement

Sets cl.predicted_origin and cl.predicted_angles
========================
*/
void CL_PredictMovement()
{
	if ( cls.state != ca_active )
	{
		return;
	}

	// don't predict if paused
	if ( cl_paused.GetBool() )
	{
		return;
	}

	if ( !cl_predict.GetBool() || ( cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION ) )
	{
		// just set angles
		for ( int i = 0; i < 3; ++i )
		{
			cl.predAngles[i] = cl.viewangles[i] + cl.frame.playerstate.pmove.delta_angles[i];
		}
		return;
	}

	int ack = cls.netchan.incoming_acknowledged;
	int current = cls.netchan.outgoing_sequence;

	// if we are too far out of date, just freeze
	if ( current - ack >= CMD_BACKUP )
	{
		if ( cl_showmiss.GetBool() )
		{
			Com_Print( "exceeded CMD_BACKUP\n" );
		}
		return;
	}

	pmove_t pm{};

	// set funcs
	pm.trace = CL_PMTrace;
	pm.pointcontents = CL_PMPointContents;
	pm.playsound = CL_PMPlaySound;

	// copy current state to pmove
	pm.s = cl.frame.playerstate.pmove;

//	SCR_DebugGraph (current - ack - 1, colorBlack);

	// run frames
	while ( ++ack < current )
	{
		int frame = ack & ( CMD_BACKUP - 1 );

		// copy over the cmd
		pm.cmd = cl.cmds[frame];

		// perform the move!
		cge->Pmove( &pm );

		// save for debug checking
		VectorCopy( pm.s.origin, cl.predicted_origins[frame] );
	}

	// calc data for smoothing stair ups
	int oldframe = ( ack - 2 ) & ( CMD_BACKUP - 1 );
	float oldz = cl.predicted_origins[oldframe][2];
	float step = pm.s.origin[2] - oldz;
	float stepFabs = fabs( step );
	if ( pm.groundentity && stepFabs > 1.0f && stepFabs < 18.0f ) // STEPSIZE
	{
		cl.predicted_step = step;
		cl.predicted_step_time = cls.realtime - ( cls.frametime * 100.0f );
	}

	cl.predMove = pm.s;

	// copy results out for rendering
	VectorCopy( pm.viewangles, cl.predAngles );
}
