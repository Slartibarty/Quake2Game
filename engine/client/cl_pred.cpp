
#include "client.h"


/*
===================
CL_CheckPredictionError
===================
*/
void CL_CheckPredictionError (void)
{
	int		frame;
	vec3_t	delta;
	int		i;
	float	len;

	if (!cl_predict->value || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
		return;

	// calculate the last usercmd_t we sent that the server has processed
	frame = cls.netchan.incoming_acknowledged;
	frame &= (CMD_BACKUP-1);

	// compare what the server returned with what we had predicted it to be
	VectorSubtract (cl.frame.playerstate.pmove.origin, cl.predicted_origins[frame], delta);

	// save the prediction error for interpolation
	len = fabs(delta[0]) + fabs(delta[1]) + fabs(delta[2]);
	if (len > 640)	// 80 world units
	{	// a teleport or something
		VectorClear (cl.prediction_error);
	}
	else
	{
		if (cl_showmiss->value && (delta[0] || delta[1] || delta[2]) )
			Com_Printf ("prediction miss on %i: %f\n", cl.frame.serverframe, 
			delta[0] + delta[1] + delta[2]);

		VectorCopy (cl.frame.playerstate.pmove.origin, cl.predicted_origins[frame]);

		// save for error itnerpolation
		for (i=0 ; i<3 ; i++)
			cl.prediction_error[i] = delta[i];
	}
}


/*
====================
CL_ClipMoveToEntities

====================
*/
void CL_ClipMoveToEntities ( vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, trace_t *tr )
{
	int			i, x, zd, zu;
	trace_t		trace;
	int			headnode;
	float		*angles;
	entity_state_t	*ent;
	int			num;
	cmodel_t		*cmodel;
	vec3_t		bmins, bmaxs;

	for (i=0 ; i<cl.frame.num_entities ; i++)
	{
		num = (cl.frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_parse_entities[num];

		if (!ent->solid)
			continue;

		if (ent->number == cl.playernum+1)
			continue;

		if (ent->solid == 31)
		{	// special value for bmodel
			cmodel = cl.model_clip[ent->modelindex];
			if (!cmodel)
				continue;
			headnode = cmodel->headnode;
			angles = ent->angles;
		}
		else
		{	// encoded bbox
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

		trace = CM_TransformedBoxTrace (start, end,
			mins, maxs, headnode,  MASK_PLAYERSOLID,
			ent->origin, angles);

		if (trace.allsolid || trace.startsolid ||
		trace.fraction < tr->fraction)
		{
			trace.ent = (edict_t *)ent;
		 	if (tr->startsolid)
			{
				*tr = trace;
				tr->startsolid = true;
			}
			else
				*tr = trace;
		}
		else if (trace.startsolid)
			tr->startsolid = true;
	}
}


/*
================
CL_PMTrace
================
*/
trace_t		CL_PMTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	trace_t	t;

	// check against world
	t = CM_BoxTrace (start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (t.fraction < 1.0)
		t.ent = (edict_t *)1;

	// check all other solid models
	CL_ClipMoveToEntities (start, mins, maxs, end, &t);

	return t;
}

int		CL_PMpointcontents (vec3_t point)
{
	int			i;
	entity_state_t	*ent;
	int			num;
	cmodel_t		*cmodel;
	int			contents;

	contents = CM_PointContents (point, 0);

	for (i=0 ; i<cl.frame.num_entities ; i++)
	{
		num = (cl.frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_parse_entities[num];

		if (ent->solid != 31) // special value for bmodel
			continue;

		cmodel = cl.model_clip[ent->modelindex];
		if (!cmodel)
			continue;

		contents |= CM_TransformedPointContents (point, cmodel->headnode, ent->origin, ent->angles);
	}

	return contents;
}


//static edict_t *enthack;

static void PM_PlaySound( const char *sample, float volume )
{
}

/*
=================
CL_PredictMovement

Sets cl.predicted_origin and cl.predicted_angles
=================
*/
void CL_PredictMovement (void)
{
	int			ack, current;
	int			frame;
	int			oldframe;
	usercmd_t	*cmd;
	pmove_t		pm;
	int			i;
	int			step;
	int			oldz;

	if (cls.state != ca_active)
		return;

	if (cl_paused->value)
		return;

	if (!cl_predict->value || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
	{	// just set angles
		for (i=0 ; i<3 ; i++)
		{
			cl.predicted_angles[i] = cl.viewangles[i] + cl.frame.playerstate.pmove.delta_angles[i];
		}
		return;
	}

	ack = cls.netchan.incoming_acknowledged;
	current = cls.netchan.outgoing_sequence;

	// if we are too far out of date, just freeze
	if (current - ack >= CMD_BACKUP)
	{
		if (cl_showmiss->value)
			Com_Printf ("exceeded CMD_BACKUP\n");
		return;	
	}

	// copy current state to pmove
	memset (&pm, 0, sizeof(pm));
	pm.trace = CL_PMTrace;
	pm.pointcontents = CL_PMpointcontents;
	pm.playsound = PM_PlaySound;

	//pm_airaccelerate = (float)atof(cl.configstrings[CS_AIRACCEL]);

	pm.s = cl.frame.playerstate.pmove;

//	SCR_DebugGraph (current - ack - 1, 0);

	frame = 0;

	// run frames
	while (++ack < current)
	{
		frame = ack & (CMD_BACKUP-1);
		cmd = &cl.cmds[frame];

		pm.cmd = *cmd;
		Pmove (&pm);

		// save for debug checking
		_VectorCopy (pm.s.origin, cl.predicted_origins[frame]);
	}

	oldframe = (ack-2) & (CMD_BACKUP-1);
	oldz = cl.predicted_origins[oldframe][2];
	step = pm.s.origin[2] - oldz;
	if (step > 63 && step < 160 && (pm.s.pm_flags & PMF_ON_GROUND) )
	{
		cl.predicted_step = step;
		cl.predicted_step_time = cls.realtime - cls.frametime * 500;
	}


	// copy results out for rendering
	cl.predicted_origin[0] = pm.s.origin[0];
	cl.predicted_origin[1] = pm.s.origin[1];
	cl.predicted_origin[2] = pm.s.origin[2];

	VectorCopy (pm.viewangles, cl.predicted_angles);
}
