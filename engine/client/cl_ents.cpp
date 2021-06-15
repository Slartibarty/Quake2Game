// cl_ents.c -- entity parsing and management

#include "cl_local.h"

static constexpr float MaxViewmodelLag = 1.5f;
static_assert( MaxViewmodelLag > 0.0f );

static constexpr float cl_bobcycle = 0.8f;
static constexpr float cl_bob = 0.01f;
static constexpr float cl_bobup = 0.5f;

static constexpr float cl_rollangle = 2.0f;
static constexpr float cl_rollspeed = 200.0f;

//extern model_t *cl_mod_powerscreen;

/*
===================================================================================================

	Frame parsing

===================================================================================================
*/

/*
========================
CL_ParseEntityBits

Returns the entity number and the header bits
========================
*/
int CL_ParseEntityBits( uint *bits )
{
	uint	b, total;
	int		number;

	total = MSG_ReadByte( &net_message );
	if ( total & U_MOREBITS1 )
	{
		b = MSG_ReadByte( &net_message );
		total |= b << 8;
	}
	if ( total & U_MOREBITS2 )
	{
		b = MSG_ReadByte( &net_message );
		total |= b << 16;
	}
	if ( total & U_MOREBITS3 )
	{
		b = MSG_ReadByte( &net_message );
		total |= b << 24;
	}

	if ( total & U_NUMBER16 ) {
		number = MSG_ReadShort( &net_message );
	} else {
		number = MSG_ReadByte( &net_message );
	}

	*bits = total;

	return number;
}

/*
========================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
========================
*/
void CL_ParseDelta( entity_state_t *from, entity_state_t *to, int number, uint bits )
{
	// set everything to the state we are delta'ing from
	*to = *from;

	VectorCopy( from->origin, to->old_origin );
	to->number = number;

	if ( bits & U_MODEL ) {
		to->modelindex = MSG_ReadByte( &net_message );
	}
	if ( bits & U_MODEL2 ) {
		to->modelindex2 = MSG_ReadByte( &net_message );
	}
	if ( bits & U_MODEL3 ) {
		to->modelindex3 = MSG_ReadByte( &net_message );
	}
	if ( bits & U_MODEL4 ) {
		to->modelindex4 = MSG_ReadByte( &net_message );
	}

	if ( bits & U_FRAME8 ) {
		to->frame = MSG_ReadByte( &net_message );
	}
	if ( bits & U_FRAME16 ) {
		to->frame = MSG_ReadShort( &net_message );
	}

	if ( ( bits & U_SKIN8 ) && ( bits & U_SKIN16 ) ) {		//used for laser colors
		to->skinnum = MSG_ReadLong( &net_message );
	}
	else if ( bits & U_SKIN8 ) {
		to->skinnum = MSG_ReadByte( &net_message );
	}
	else if ( bits & U_SKIN16 ) {
		to->skinnum = MSG_ReadShort( &net_message );
	}

	if ( ( bits & ( U_EFFECTS8 | U_EFFECTS16 ) ) == ( U_EFFECTS8 | U_EFFECTS16 ) ) {
		to->effects = MSG_ReadLong( &net_message );
	}
	else if ( bits & U_EFFECTS8 ) {
		to->effects = MSG_ReadByte( &net_message );
	}
	else if ( bits & U_EFFECTS16 ) {
		to->effects = MSG_ReadShort( &net_message );
	}

	if ( ( bits & ( U_RENDERFX8 | U_RENDERFX16 ) ) == ( U_RENDERFX8 | U_RENDERFX16 ) ) {
		to->renderfx = MSG_ReadLong( &net_message );
	}
	else if ( bits & U_RENDERFX8 ) {
		to->renderfx = MSG_ReadByte( &net_message );
	}
	else if ( bits & U_RENDERFX16 ) {
		to->renderfx = MSG_ReadShort( &net_message );
	}

	if ( bits & U_ORIGIN1 ) {
		to->origin[0] = MSG_ReadCoord( &net_message );
	}
	if ( bits & U_ORIGIN2 ) {
		to->origin[1] = MSG_ReadCoord( &net_message );
	}
	if ( bits & U_ORIGIN3 ) {
		to->origin[2] = MSG_ReadCoord( &net_message );
	}

	if ( bits & U_ANGLE1 ) {
		to->angles[0] = MSG_ReadAngle( &net_message );
	}
	if ( bits & U_ANGLE2 ) {
		to->angles[1] = MSG_ReadAngle( &net_message );
	}
	if ( bits & U_ANGLE3 ) {
		to->angles[2] = MSG_ReadAngle( &net_message );
	}

	if ( bits & U_OLDORIGIN ) {
		MSG_ReadPos( &net_message, to->old_origin );
	}

	if ( bits & U_SOUND ) {
		to->sound = MSG_ReadByte( &net_message );
	}

	if ( bits & U_EVENT ) {
		to->event = MSG_ReadByte( &net_message );
	} else {
		to->event = 0;
	}

	if ( bits & U_SOLID ) {
		to->solid = MSG_ReadShort( &net_message );
	}
}

/*
========================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
========================
*/
static void CL_DeltaEntity( clSnapshot_t *frame, int newnum, entity_state_t *old, uint bits )
{
	centity_t *ent;
	entity_state_t *state;

	ent = &cl_entities[newnum];

	state = &cl_parse_entities[cl.parse_entities & ( MAX_PARSE_ENTITIES - 1 )];
	cl.parse_entities++;
	frame->num_entities++;

	CL_ParseDelta( old, state, newnum, bits );

	// some data changes will force no lerping
	if ( state->modelindex != ent->current.modelindex
		|| state->modelindex2 != ent->current.modelindex2
		|| state->modelindex3 != ent->current.modelindex3
		|| state->modelindex4 != ent->current.modelindex4
		|| fabsf( state->origin[0] - ent->current.origin[0] ) > 512.0f
		|| fabsf( state->origin[1] - ent->current.origin[1] ) > 512.0f
		|| fabsf( state->origin[2] - ent->current.origin[2] ) > 512.0f
		|| state->event == EV_PLAYER_TELEPORT
		|| state->event == EV_OTHER_TELEPORT )
	{
		ent->serverframe = -99;
	}

	if ( ent->serverframe != cl.frame.serverframe - 1 )
	{
		// wasn't in last update, so initialize some things
		ent->trailcount = 1024;		// for diminishing rocket / grenade trails
		// duplicate the current state so lerping doesn't hurt anything
		ent->prev = *state;
		if ( state->event == EV_OTHER_TELEPORT )
		{
			VectorCopy( state->origin, ent->prev.origin );
			VectorCopy( state->origin, ent->lerp_origin );
		}
		else
		{
			VectorCopy( state->old_origin, ent->prev.origin );
			VectorCopy( state->old_origin, ent->lerp_origin );
		}
	}
	else
	{
		// shuffle the last state to previous
		ent->prev = ent->current;
	}

	ent->serverframe = cl.frame.serverframe;
	ent->current = *state;
}

/*
========================
CL_ParsePacketEntities

An svc_packetentities has just been parsed, deal with the
rest of the data stream.
========================
*/
static void CL_ParsePacketEntities( clSnapshot_t *oldframe, clSnapshot_t *newframe )
{
	int			newnum;
	uint		bits;
	entity_state_t *oldstate;
	int			oldindex, oldnum;

	newframe->parse_entities = cl.parse_entities;
	newframe->num_entities = 0;

	// delta from the entities present in oldframe
	oldindex = 0;
	if ( !oldframe )
	{
		oldnum = 99999;
	}
	else
	{
		if ( oldindex >= oldframe->num_entities )
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &cl_parse_entities[( oldframe->parse_entities + oldindex ) & ( MAX_PARSE_ENTITIES - 1 )];
			oldnum = oldstate->number;
		}
	}

	while ( 1 )
	{
		newnum = CL_ParseEntityBits( &bits );
		if ( newnum >= MAX_EDICTS ) {
			Com_Errorf( "CL_ParsePacketEntities: bad number:%i", newnum );
		}

		if ( net_message.readcount > net_message.cursize ) {
			Com_Error( "CL_ParsePacketEntities: end of message" );
		}

		if ( !newnum ) {
			break;
		}

		while ( oldnum < newnum )
		{
			// one or more entities from the old packet are unchanged
			if ( cl_shownet->GetInt32() == 3 ) {
				Com_Printf( "   unchanged: %i\n", oldnum );
			}
			CL_DeltaEntity( newframe, oldnum, oldstate, 0 );

			oldindex++;

			if ( oldindex >= oldframe->num_entities )
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl_parse_entities[( oldframe->parse_entities + oldindex ) & ( MAX_PARSE_ENTITIES - 1 )];
				oldnum = oldstate->number;
			}
		}

		if ( bits & U_REMOVE )
		{
			// the entity present in oldframe is not in the current frame
			if ( cl_shownet->GetInt64() == 3 ) {
				Com_Printf( "   remove: %i\n", newnum );
			}
			if ( oldnum != newnum ) {
				Com_Printf( "U_REMOVE: oldnum != newnum\n" );
			}

			oldindex++;

			if ( oldindex >= oldframe->num_entities )
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl_parse_entities[( oldframe->parse_entities + oldindex ) & ( MAX_PARSE_ENTITIES - 1 )];
				oldnum = oldstate->number;
			}
			continue;
		}

		if ( oldnum == newnum )
		{	// delta from previous state
			if ( cl_shownet->GetInt32() == 3 ) {
				Com_Printf( "   delta: %i\n", newnum );
			}
			CL_DeltaEntity( newframe, newnum, oldstate, bits );

			oldindex++;

			if ( oldindex >= oldframe->num_entities )
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl_parse_entities[( oldframe->parse_entities + oldindex ) & ( MAX_PARSE_ENTITIES - 1 )];
				oldnum = oldstate->number;
			}
			continue;
		}

		if ( oldnum > newnum )
		{
			// delta from baseline
			if ( cl_shownet->GetInt32() == 3 ) {
				Com_Printf( "   baseline: %i\n", newnum );
			}
			CL_DeltaEntity( newframe, newnum, &cl_entities[newnum].baseline, bits );
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while ( oldnum != 99999 )
	{
		// one or more entities from the old packet are unchanged
		if ( cl_shownet->GetInt32() == 3 ) {
			Com_Printf( "   unchanged: %i\n", oldnum );
		}
		CL_DeltaEntity( newframe, oldnum, oldstate, 0 );

		oldindex++;

		if ( oldindex >= oldframe->num_entities )
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &cl_parse_entities[( oldframe->parse_entities + oldindex ) & ( MAX_PARSE_ENTITIES - 1 )];
			oldnum = oldstate->number;
		}
	}
}

/*
========================
CL_ParsePlayerstate
========================
*/
void CL_ParsePlayerstate( clSnapshot_t *oldframe, clSnapshot_t *newframe )
{
	int			flags;
	player_state_t *state;
	int			i;
	int			statbits;

	state = &newframe->playerstate;

	// clear to old value before delta parsing
	if ( oldframe ) {
		*state = oldframe->playerstate;
	} else {
		memset( state, 0, sizeof( *state ) );
	}

	flags = MSG_ReadShort( &net_message );

	//
	// parse the pmove_state_t
	//
	if ( flags & PS_M_TYPE ) {
		state->pmove.pm_type = (pmType_t)MSG_ReadByte( &net_message );
	}

	if ( flags & PS_M_ORIGIN )
	{
		state->pmove.origin[0] = MSG_ReadFloat( &net_message );
		state->pmove.origin[1] = MSG_ReadFloat( &net_message );
		state->pmove.origin[2] = MSG_ReadFloat( &net_message );
	}

	if ( flags & PS_M_VELOCITY )
	{
		state->pmove.velocity[0] = MSG_ReadFloat( &net_message );
		state->pmove.velocity[1] = MSG_ReadFloat( &net_message );
		state->pmove.velocity[2] = MSG_ReadFloat( &net_message );
	}

	if ( flags & PS_M_TIME ) {
		state->pmove.pm_time = MSG_ReadByte( &net_message );
	}

	if ( flags & PS_M_FLAGS ) {
		state->pmove.pm_flags = MSG_ReadByte( &net_message );
	}

	if ( flags & PS_M_GRAVITY ) {
		state->pmove.gravity = MSG_ReadShort( &net_message );
	}

	if ( flags & PS_M_DELTA_ANGLES )
	{
		state->pmove.delta_angles[0] = MSG_ReadFloat( &net_message );
		state->pmove.delta_angles[1] = MSG_ReadFloat( &net_message );
		state->pmove.delta_angles[2] = MSG_ReadFloat( &net_message );
	}

	if ( cl.attractloop ) {
		// demo playback
		state->pmove.pm_type = PM_FREEZE;
	}

	//
	// parse the rest of the player_state_t
	//
	if ( flags & PS_VIEWOFFSET )
	{
		state->viewoffset[0] = MSG_ReadFloat( &net_message );
		state->viewoffset[1] = MSG_ReadFloat( &net_message );
		state->viewoffset[2] = MSG_ReadFloat( &net_message );
	}

	if ( flags & PS_VIEWANGLES )
	{
		state->viewangles[0] = MSG_ReadFloat( &net_message );
		state->viewangles[1] = MSG_ReadFloat( &net_message );
		state->viewangles[2] = MSG_ReadFloat( &net_message );
	}

	if ( flags & PS_KICKANGLES )
	{
		state->kick_angles[0] = MSG_ReadFloat( &net_message );
		state->kick_angles[1] = MSG_ReadFloat( &net_message );
		state->kick_angles[2] = MSG_ReadFloat( &net_message );
	}

	if ( flags & PS_WEAPONINDEX )
	{
		state->gunindex = MSG_ReadByte( &net_message );
	}

	if ( flags & PS_WEAPONFRAME )
	{
		state->gunframe = MSG_ReadByte( &net_message );
		state->gunoffset[0] = MSG_ReadChar( &net_message ) * 0.25f;
		state->gunoffset[1] = MSG_ReadChar( &net_message ) * 0.25f;
		state->gunoffset[2] = MSG_ReadChar( &net_message ) * 0.25f;
		state->gunangles[0] = MSG_ReadChar( &net_message ) * 0.25f;
		state->gunangles[1] = MSG_ReadChar( &net_message ) * 0.25f;
		state->gunangles[2] = MSG_ReadChar( &net_message ) * 0.25f;
	}

	if ( flags & PS_BLEND )
	{
		state->blend[0] = MSG_ReadByte( &net_message ) / 255.0f;
		state->blend[1] = MSG_ReadByte( &net_message ) / 255.0f;
		state->blend[2] = MSG_ReadByte( &net_message ) / 255.0f;
		state->blend[3] = MSG_ReadByte( &net_message ) / 255.0f;
	}

	if ( flags & PS_FOV ) {
		state->fov = (float)MSG_ReadByte( &net_message );
	}

	if ( flags & PS_RDFLAGS ) {
		state->rdflags = MSG_ReadByte( &net_message );
	}

	// parse stats
	statbits = MSG_ReadLong( &net_message );
	for ( i = 0; i < MAX_STATS; i++ )
	{
		if ( statbits & ( 1 << i ) ) {
			state->stats[i] = MSG_ReadShort( &net_message );
		}
	}
}

/*
========================
CL_FireEntityEvents
========================
*/
void CL_FireEntityEvents( clSnapshot_t *frame )
{
	entity_state_t *s1;
	int pnum, num;

	for ( pnum = 0; pnum < frame->num_entities; pnum++ )
	{
		num = ( frame->parse_entities + pnum ) & ( MAX_PARSE_ENTITIES - 1 );
		s1 = &cl_parse_entities[num];
		if ( s1->event ) {
			cge->EntityEvent( s1 );
		}

		// EF_TELEPORTER acts like an event, but is not cleared each frame
		if ( s1->effects & EF_TELEPORTER ) {
			cge->TeleporterParticles( s1 );
		}
	}
}

/*
========================
CL_ParseFrame
========================
*/
void CL_ParseFrame()
{
	int cmd;
	int len;
	clSnapshot_t *old;

	memset( &cl.frame, 0, sizeof( cl.frame ) );

	cl.frame.serverframe = MSG_ReadLong( &net_message );
	cl.frame.deltaframe = MSG_ReadLong( &net_message );
	cl.frame.servertime = cl.frame.serverframe * 100;

	// BIG HACK to let old demos continue to work
	if ( cls.serverProtocol != 26 ) {
		cl.surpressCount = MSG_ReadByte( &net_message );
	}

	if ( cl_shownet->GetInt32() == 3 ) {
		Com_Printf( "   frame:%i  delta:%i\n", cl.frame.serverframe, cl.frame.deltaframe );
	}

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message 
	if ( cl.frame.deltaframe <= 0 )
	{
		cl.frame.valid = true;		// uncompressed frame
		old = nullptr;
		cls.demowaiting = false;	// we can start recording now
	}
	else
	{
		old = &cl.frames[cl.frame.deltaframe & UPDATE_MASK];
		if ( !old->valid )
		{
			// should never happen
			Com_Print( "Delta from invalid frame (not supposed to happen!).\n" );
		}
		if ( old->serverframe != cl.frame.deltaframe )
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Com_Print( "Delta frame too old.\n" );
		}
		else if ( cl.parse_entities - old->parse_entities > MAX_PARSE_ENTITIES - 128 )
		{
			Com_Print( "Delta parse_entities too old.\n" );
		}
		else
		{
			// valid delta parse
			cl.frame.valid = true;
		}
	}

	// clamp time 
	if ( cl.time > cl.frame.servertime ) {
		cl.time = cl.frame.servertime;
	}
	else if ( cl.time < cl.frame.servertime - 100 ) {
		cl.time = cl.frame.servertime - 100;
	}

	// read areabits
	len = MSG_ReadByte( &net_message );
	MSG_ReadData( &net_message, &cl.frame.areabits, len );

	// read playerinfo
	cmd = MSG_ReadByte( &net_message );
	SHOWNET( svc_strings[cmd] );
	if ( cmd != svc_playerinfo ) {
		Com_Error( "CL_ParseFrame: not playerinfo" );
	}
	CL_ParsePlayerstate( old, &cl.frame );

	// read packet entities
	cmd = MSG_ReadByte( &net_message );
	SHOWNET( svc_strings[cmd] );
	if ( cmd != svc_packetentities ) {
		Com_Error( "CL_ParseFrame: not packetentities" );
	}
	CL_ParsePacketEntities( old, &cl.frame );

	// save the frame off in the backup array for later delta comparisons
	cl.frames[cl.frame.serverframe & UPDATE_MASK] = cl.frame;

	if ( cl.frame.valid )
	{
		// getting a valid frame message ends the connection process
		if ( cls.state != ca_active )
		{
			cls.state = ca_active;
			cl.force_refdef = true;
			VectorCopy( cl.frame.playerstate.pmove.origin, cl.predMove.origin );
			VectorCopy( cl.frame.playerstate.viewangles, cl.predAngles );

			if ( cls.disable_servercount != cl.servercount && cl.refresh_prepped ) {
				// get rid of loading plaque
				SCR_EndLoadingPlaque();
			}
		}
		cl.sound_prepped = true;	// can start mixing ambient sounds

		// fire entity events
		CL_FireEntityEvents( &cl.frame );
		CL_CheckPredictionError();
	}
}

/*
===================================================================================================

	Interpolate between frames to get rendering parms

===================================================================================================
*/

/*
========================
CL_AddPacketEntities
========================
*/
static void CL_AddPacketEntities( clSnapshot_t *frame )
{
	entity_t			ent;
	entity_state_t *	s1;
	float				autorotate;
	int					i;
	int					pnum;
	centity_t *			cent;
	int					autoanim;
	clientinfo_t *		ci;
	unsigned int		effects, renderfx;

	// bonus items rotate at a fixed rate
	autorotate = anglemod( static_cast<float>( cl.time / 10 ) );

	// brush models can auto animate their frames
	autoanim = 2 * MS2SEC( cl.time );

	memset( &ent, 0, sizeof( ent ) );

	for ( pnum = 0; pnum < frame->num_entities; pnum++ )
	{
		s1 = &cl_parse_entities[( frame->parse_entities + pnum ) & ( MAX_PARSE_ENTITIES - 1 )];

		cent = &cl_entities[s1->number];

		effects = s1->effects;
		renderfx = s1->renderfx;

		// set frame
		if ( effects & EF_ANIM01 ) {
			ent.frame = autoanim & 1;
		}
		else if ( effects & EF_ANIM23 ) {
			ent.frame = 2 + ( autoanim & 1 );
		}
		else if ( effects & EF_ANIM_ALL ) {
			ent.frame = autoanim;
		}
		else if ( effects & EF_ANIM_ALLFAST ) {
			ent.frame = cl.time / 100;
		}
		else {
			ent.frame = s1->frame;
		}

		// quad and pent can do different things on client
		if ( effects & EF_PENT )
		{
			effects &= ~EF_PENT;
			effects |= EF_COLOR_SHELL;
			renderfx |= RF_SHELL_RED;
		}

		if ( effects & EF_QUAD )
		{
			effects &= ~EF_QUAD;
			effects |= EF_COLOR_SHELL;
			renderfx |= RF_SHELL_BLUE;
		}
//======
// PMM
		if ( effects & EF_DOUBLE )
		{
			effects &= ~EF_DOUBLE;
			effects |= EF_COLOR_SHELL;
			renderfx |= RF_SHELL_DOUBLE;
		}

		if ( effects & EF_HALF_DAMAGE )
		{
			effects &= ~EF_HALF_DAMAGE;
			effects |= EF_COLOR_SHELL;
			renderfx |= RF_SHELL_HALF_DAM;
		}
// pmm
//======
		ent.oldframe = cent->prev.frame;
		ent.backlerp = 1.0f - cl.lerpfrac;

		if ( renderfx & ( RF_FRAMELERP | RF_BEAM ) )
		{
			// step origin discretely, because the frames
			// do the animation properly
			VectorCopy( cent->current.origin, ent.origin );
			VectorCopy( cent->current.old_origin, ent.oldorigin );
		}
		else
		{
			// interpolate origin
			for ( i = 0; i < 3; i++ )
			{
				ent.origin[i] = ent.oldorigin[i] = cent->prev.origin[i] + cl.lerpfrac *
					( cent->current.origin[i] - cent->prev.origin[i] );
			}
		}

		// create a new entity

		// tweak the color of beams
		if ( renderfx & RF_BEAM )
		{
			// the four beam colors are encoded in 32 bits of skinnum (hack)
			ent.alpha = 0.30f;
			ent.skinnum = ( s1->skinnum >> ( ( rand() % 4 ) * 8 ) ) & 0xff;
			ent.model = nullptr;
		}
		else
		{
			// set skin
			if ( s1->modelindex == 255 )
			{
				// use custom player skin
				ent.skinnum = 0;
				ci = &cl.clientinfo[s1->skinnum & 0xff];
				ent.skin = ci->skin;
				ent.model = ci->model;
				if ( !ent.skin || !ent.model )
				{
					ent.skin = cl.baseclientinfo.skin;
					ent.model = cl.baseclientinfo.model;
				}
			}
			else
			{
				ent.skinnum = s1->skinnum;
				ent.skin = nullptr;
				ent.model = cl.model_draw[s1->modelindex];
			}
		}

		// only used for black hole model right now, FIXME: do better
		if ( renderfx == RF_TRANSLUCENT ) {
			ent.alpha = 0.70f;
		}

		// render effects (fullbright, translucent, etc)
		if ( ( effects & EF_COLOR_SHELL ) ) {
			// renderfx go on color shell entity
			ent.flags = 0;
		} else {
			ent.flags = renderfx;
		}

		// calculate angles
		if ( effects & EF_ROTATE )
		{
			// some bonus items auto-rotate
			ent.angles[0] = 0;
			ent.angles[1] = autorotate;
			ent.angles[2] = 0;
		}
		// RAFAEL
		else if ( effects & EF_SPINNINGLIGHTS )
		{
			ent.angles[0] = 0;
			ent.angles[1] = anglemod( (float)( cl.time / 2 ) ) + s1->angles[1];
			ent.angles[2] = 180;
			{
				vec3_t forward;
				vec3_t start;

				AngleVectors( ent.angles, forward, NULL, NULL );
				VectorMA( ent.origin, 64, forward, start );
				V_AddDLight( start, 100, 1, 0, 0 );
			}
		}
		else
		{	// interpolate angles
			float a1, a2;

			for ( i = 0; i < 3; i++ )
			{
				a1 = cent->current.angles[i];
				a2 = cent->prev.angles[i];
				ent.angles[i] = LerpAngle( a2, a1, cl.lerpfrac );
			}
		}

		if ( s1->number == cl.playernum + 1 )
		{
			ent.flags |= RF_VIEWERMODEL;	// only draw from mirrors
			// FIXME: still pass to refresh

			if ( effects & EF_FLAG1 ) {
				V_AddDLight( ent.origin, 225.0f, 1.0f, 0.1f, 0.1f );
			}
			else if ( effects & EF_FLAG2 ) {
				V_AddDLight( ent.origin, 225.0f, 0.1f, 0.1f, 1.0f );
			}
			// START PGM
			else if ( effects & EF_TAGTRAIL ) {
				V_AddDLight( ent.origin, 225.0f, 1.0f, 1.0f, 0.0f );
			}
			else if ( effects & EF_TRACKERTRAIL ) {
				V_AddDLight( ent.origin, 225.0f, -1.0f, -1.0f, -1.0f );
			}
			// END PGM

			continue;
		}

		// if set to invisible, skip
		if ( !s1->modelindex ) {
			continue;
		}

		if ( effects & EF_BFG )
		{
			ent.flags |= RF_TRANSLUCENT;
			ent.alpha = 0.30f;
		}

		// RAFAEL
		if ( effects & EF_PLASMA )
		{
			ent.flags |= RF_TRANSLUCENT;
			ent.alpha = 0.6f;
		}

		if ( effects & EF_SPHERETRANS )
		{
			ent.flags |= RF_TRANSLUCENT;
			// PMM - *sigh*  yet more EF overloading
			if ( effects & EF_TRACKERTRAIL ) {
				ent.alpha = 0.6f;
			} else {
				ent.alpha = 0.3f;
			}
		}
//pmm

		// add to refresh list
		V_AddEntity( &ent );

		// color shells generate a seperate entity for the main model
		if ( effects & EF_COLOR_SHELL )
		{
#if 0
			// PMM - at this point, all of the shells have been handled
			// if we're in the rogue pack, set up the custom mixing, otherwise just
			// keep going
//			if(Developer_searchpath() == 2)
//			{
				// all of the solo colors are fine.  we need to catch any of the combinations that look bad
				// (double & half) and turn them into the appropriate color, and make double/quad something special
			if ( renderfx & RF_SHELL_HALF_DAM )
			{
				if ( Developer_searchpath() == 2 )
				{
					// ditch the half damage shell if any of red, blue, or double are on
					if ( renderfx & ( RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE ) )
						renderfx &= ~RF_SHELL_HALF_DAM;
				}
			}

			if ( renderfx & RF_SHELL_DOUBLE )
			{
				if ( Developer_searchpath() == 2 )
				{
					// lose the yellow shell if we have a red, blue, or green shell
					if ( renderfx & ( RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_GREEN ) )
						renderfx &= ~RF_SHELL_DOUBLE;
					// if we have a red shell, turn it to purple by adding blue
					if ( renderfx & RF_SHELL_RED )
						renderfx |= RF_SHELL_BLUE;
					// if we have a blue shell (and not a red shell), turn it to cyan by adding green
					else if ( renderfx & RF_SHELL_BLUE )
					{
						// go to green if it's on already, otherwise do cyan (flash green)
						if ( renderfx & RF_SHELL_GREEN )
							renderfx &= ~RF_SHELL_BLUE;
						else
							renderfx |= RF_SHELL_GREEN;
					}
				}
			}
//			}
			// pmm
#endif
			ent.flags = renderfx | RF_TRANSLUCENT;
			ent.alpha = 0.30f;
			V_AddEntity( &ent );
		}

		ent.skin = nullptr;		// never use a custom skin on others
		ent.skinnum = 0;
		ent.flags = 0;
		ent.alpha = 0;

		// duplicate for linked models
		if ( s1->modelindex2 )
		{
			if ( s1->modelindex2 == 255 )
			{
				// custom weapon
				ci = &cl.clientinfo[s1->skinnum & 0xff];
				i = ( s1->skinnum >> 8 ); // 0 is default weapon model
				if ( !cl_vwep->GetBool() || i > MAX_CLIENTWEAPONMODELS - 1 ) {
					i = 0;
				}
				ent.model = ci->weaponmodel[i];
				if ( !ent.model )
				{
					if ( i != 0 ) {
						ent.model = ci->weaponmodel[0];
					}
					if ( !ent.model ) {
						ent.model = cl.baseclientinfo.weaponmodel[0];
					}
				}
			}
			else
				ent.model = cl.model_draw[s1->modelindex2];

			V_AddEntity( &ent );

			//PGM - make sure these get reset.
			ent.flags = 0;
			ent.alpha = 0;
			//PGM
		}
		if ( s1->modelindex3 )
		{
			ent.model = cl.model_draw[s1->modelindex3];
			V_AddEntity( &ent );
		}
		if ( s1->modelindex4 )
		{
			ent.model = cl.model_draw[s1->modelindex4];
			V_AddEntity( &ent );
		}

		if ( effects & EF_POWERSCREEN )
		{
#if SLARTHACK
			ent.model = cl_mod_powerscreen;
			ent.oldframe = 0;
			ent.frame = 0;
			ent.flags |= ( RF_TRANSLUCENT | RF_SHELL_GREEN );
			ent.alpha = 0.30f;
			V_AddEntity( &ent );
#endif
		}

		// add automatic particle trails
		if ( ( effects & ~EF_ROTATE ) )
		{
			cge->RunParticles( i, effects, cent, &ent, s1 );
		}

		VectorCopy( ent.origin, cent->lerp_origin );
	}
}

#define	HL2_BOB_CYCLE_MIN	1.0f
#define	HL2_BOB_CYCLE_MAX	0.45f
#define	HL2_BOB				0.002f
#define	HL2_BOB_UP			0.5f

inline float fsel( float a, float b, float c )
{
	return a >= 0.0f ? b : c;
}

inline float RemapVal( float val, float A, float B, float C, float D )
{
	if ( A == B ) {
		return fsel( val - B, D, C );
	}
	return C + ( D - C ) * ( val - A ) / ( B - A );
}

static void CL_CalcViewmodelBob( vec3_t velocity, float &verticalBob, float &lateralBob )
{
	static float bobtime;
	float cycle;
	
	if ( cls.frametime == 0.0f )
	{
		return;
	}

	float speed = sqrt( velocity[0]*velocity[0] + velocity[1]*velocity[1] );

	speed = Clamp( speed, -320.0f, 320.0f );

	float bob_offset = RemapVal( speed, 0.0f, 320.0f, 0.0f, 1.0f );
	
	bobtime += cls.frametime * bob_offset;

	//Calculate the vertical bob
	cycle = bobtime - (int)( bobtime/HL2_BOB_CYCLE_MAX )*HL2_BOB_CYCLE_MAX;
	cycle /= HL2_BOB_CYCLE_MAX;

	if ( cycle < HL2_BOB_UP )
	{
		cycle = M_PI_F * cycle / HL2_BOB_UP;
	}
	else
	{
		cycle = M_PI_F + M_PI_F*( cycle-HL2_BOB_UP )/( 1.0f - HL2_BOB_UP );
	}
	
	verticalBob = speed*0.005f;
	verticalBob = verticalBob*0.3f + verticalBob*0.7f*sin( cycle );
	verticalBob = Clamp( verticalBob, -7.0f, 4.0f );

	//Calculate the lateral bob
	cycle = bobtime - (int)( bobtime/HL2_BOB_CYCLE_MAX*2.0f )*HL2_BOB_CYCLE_MAX*2.0f;
	cycle /= HL2_BOB_CYCLE_MAX*2.0f;

	if ( cycle < HL2_BOB_UP )
	{
		cycle = M_PI_F * cycle / HL2_BOB_UP;
	}
	else
	{
		cycle = M_PI_F + M_PI_F*( cycle-HL2_BOB_UP )/( 1.0f - HL2_BOB_UP );
	}

	lateralBob = speed*0.005f;
	lateralBob = lateralBob*0.3f + lateralBob*0.7f*sin( cycle );
	lateralBob = Clamp( lateralBob, -7.0f, 4.0f );
}

static void CL_AddViewmodelBob( vec3_t origin, vec3_t angles )
{
	float verticalBob, lateralBob;
	CL_CalcViewmodelBob( cl.predMove.velocity, verticalBob, lateralBob );

	// Apply bob, but scaled down to 40%
	VectorMA( origin, verticalBob * 0.1f, cl.v_forward, origin );

	// Z bob a bit more
	origin[2] += verticalBob * 0.1f;

	// bob the angles
	angles[ROLL] += verticalBob * 0.5f;
	angles[PITCH] -= verticalBob * 0.4f;

	angles[YAW] -= lateralBob * 0.3f;

	VectorMA( origin, lateralBob * 0.8f, cl.v_right, origin );
}

static void CL_CalcViewModelLag( vec3_t origin, const vec3_t angles )
{
	static vec3_t lastFacing;

	vec3_t forward;
	AngleVectors( angles, forward, nullptr, nullptr );

	if ( cls.frametime != 0.0f )
	{
		vec3_t difference;
		VectorSubtract( forward, lastFacing, difference );

		float speed = 5.0f;

		// If we start to lag too far behind, we'll increase the "catch up" speed.  Solves the problem with fast cl_yawspeed, m_yaw or joysticks
		//  rotating quickly.  The old code would slam lastfacing with origin causing the viewmodel to pop to a new position
		float distance = VectorLength( difference );
		if ( distance > MaxViewmodelLag && MaxViewmodelLag > 0.0f )
		{
			speed *= distance / MaxViewmodelLag;
		}

		VectorMA( lastFacing, speed * cls.frametime, difference, lastFacing );
		VectorNormalize( lastFacing );

		VectorNegate( difference, difference );
		VectorMA( origin, 5.0f, difference, origin );
	}

	vec3_t oldForward; // DEBUG
	VectorCopy( forward, oldForward ); // DEBUG

	vec3_t right, up;
	AngleVectors( angles, forward, right, up );

	assert( VectorCompare( forward, oldForward ) );

	float pitch = angles[PITCH];
	if ( pitch > 180.0f ) {
		pitch -= 360.0f;
	}
	else if ( pitch < -180.0f ) {
		pitch += 360.0f;
	}

	VectorMA( origin, -pitch * 0.035f, forward, origin );
	VectorMA( origin, -pitch * 0.03f, right, origin );
	VectorMA( origin, -pitch * 0.02f, up, origin );
}

/*
========================
CL_AddViewWeapon

Adds the viewmodel entity
========================
*/
static void CL_AddViewWeapon( player_state_t *ps, player_state_t *ops, float bob )
{
	entity_t view;

	// allow the gun to be completely removed
	if ( !cl_drawviewmodel->GetBool() ) {
		return;
	}

	memset( &view, 0, sizeof( view ) );

	if ( gun_model ) {
		// development tool
		view.model = gun_model;
	} else {
		view.model = cl.model_draw[ps->gunindex];
	}

	if ( !view.model ) {
		return;
	}

#if 0

	for ( int i = 0; i < 3; i++ )
	{
		view.origin[i] = cl.refdef.vieworg[i] + ( bob * 0.4f * cl.v_forward[i] );
		view.angles[i] = cl.refdef.viewangles[i];
	}

	// throw in a little tilt
	view.angles[PITCH] -= bob * 0.3f;
	view.angles[YAW]   -= bob * 0.5f;
	view.angles[ROLL]  -= bob * 1.0f;

	// pushing the view origin down off of the same X/Z plane as the ent's origin will give the
	// gun a very nice 'shifting' effect when the player looks up/down. If there is a problem
	// with view model distortion, this may be a cause. (SJB). 
	view.origin[2] -= 1.0f;

#else

	VectorCopy( cl.refdef.vieworg, view.origin );
	VectorCopy( cl.refdef.viewangles, view.angles );

	CL_AddViewmodelBob( view.origin, view.angles );
	CL_CalcViewModelLag( view.origin, view.angles );

#endif

	if ( gun_frame )
	{
		view.frame = gun_frame;		// development tool
		view.oldframe = gun_frame;	// development tool
	}
	else
	{
		view.frame = ps->gunframe;
		if ( view.frame == 0 ) {
			// just changed weapons, don't lerp from old
			view.oldframe = 0;
		} else {
			view.oldframe = ops->gunframe;
		}
	}

	view.flags = RF_MINLIGHT | RF_DEPTHHACK | RF_WEAPONMODEL;
	view.backlerp = 1.0f - cl.lerpfrac;
	VectorCopy( view.origin, view.oldorigin );	// don't lerp at all

	V_AddEntity( &view );
}

/*
========================
CL_CalcBob
========================
*/
static float CL_CalcBob( vec3_t velocity )
{
	return 0.0f;
#if 0
	static float bobtime;
	static float bob;
	float cycle;

	static int lasttime;
	
	if ( cl.time == lasttime )
	{
		// just use old value
		return bob;	
	}

	lasttime = cl.time;

	bobtime += cls.frametime;
	cycle = bobtime - (int)( bobtime / cl_bobcycle ) * cl_bobcycle;
	cycle /= cl_bobcycle;
	
	if ( cycle < cl_bobup )
	{
		cycle = M_PI_F * cycle / cl_bobup;
	}
	else
	{
		cycle = M_PI_F + M_PI_F*( cycle - cl_bobup )/( 1.0f - cl_bobup );
	}

	// bob is proportional to simulated velocity in the xy plane
	// (don't count Z, or jumping messes it up)

	bob = sqrt( velocity[0]*velocity[0] + velocity[1]*velocity[1] ) * cl_bob;
	bob = bob*0.3f + bob*0.7f*sin( cycle );
	bob = Clamp( bob, -7.0f, 4.0f );

	return bob;
#endif
}

/*
========================
CL_CalcRoll
========================
*/
static float CL_CalcRoll( vec3_t angles, vec3_t velocity )
{
	return 0.0f;
#if 0
	float sign;
	float side;
	float value;
	vec3_t right;

	AngleVectors( angles, nullptr, right, nullptr );
	side = DotProduct( velocity, right );
	sign = side < 0 ? -1 : 1;
	side = fabs( side );

	value = cl_rollangle;
	if ( side < cl_rollspeed ) {
		side = side * value / cl_rollspeed;
	} else {
		side = value;
	}

	return side * sign;
#endif
}

/*
========================
CL_CalcViewValues

Sets cl.refdef view values
========================
*/
void CL_CalcViewValues()
{
	int				i;
	float			lerp, backlerp;
	centity_t *		ent;
	clSnapshot_t *	oldframe;
	player_state_t	*ps, *ops;

	// find the previous frame to interpolate from
	ps = &cl.frame.playerstate;
	i = ( cl.frame.serverframe - 1 ) & UPDATE_MASK;
	oldframe = &cl.frames[i];
	if ( oldframe->serverframe != cl.frame.serverframe - 1 || !oldframe->valid ) {
		// previous frame was dropped or invalid
		oldframe = &cl.frame;
	}
	ops = &oldframe->playerstate;

#if 0	// SlartTodo: should be /= 8 right? This is fine at 256 but Q3 doesn't do this
	// see if the player entity was teleported this frame
	if ( fabs( ops->pmove.origin[0] - ps->pmove.origin[0] ) > 256.0f
		|| fabs( ops->pmove.origin[1] - ps->pmove.origin[1] ) > 256.0f
		|| fabs( ops->pmove.origin[2] - ps->pmove.origin[2] ) > 256.0f )
		ops = ps;		// don't interpolate
#endif

	ent = &cl_entities[cl.playernum + 1];
	lerp = cl.lerpfrac;

	// calculate the origin
	if ( cl_predict->GetBool() && !( cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION ) )
	{
		// use predicted values
		backlerp = 1.0f - lerp;

		for ( i = 0; i < 3; i++ )
		{
			cl.refdef.vieworg[i] = cl.predMove.origin[i] + ops->viewoffset[i]
				+ cl.lerpfrac * ( ps->viewoffset[i] - ops->viewoffset[i] )
				- backlerp * cl.predError[i];
		}

		// smooth out stair climbing
		uint delta = cls.realtime - cl.predicted_step_time;

		if ( delta < 100 )
		{
			// TODO: sucks
			//cl.refdef.vieworg[2] -= cl.predicted_step * float( 100 - delta ) * 0.01f;
		}
	}
	else
	{
		// just use interpolated values
		for ( i = 0; i < 3; i++ )
		{
			cl.refdef.vieworg[i] = ops->pmove.origin[i] + ops->viewoffset[i]
				+ lerp * ( ps->pmove.origin[i] + ps->viewoffset[i]
					- ( ops->pmove.origin[i] + ops->viewoffset[i] ) );
		}
	}

	float bob = CL_CalcBob( cl.predMove.velocity );

	cl.refdef.vieworg[2] += bob;

	// if not running a demo or on a locked frame, add the local angle movement
	if ( cl.frame.playerstate.pmove.pm_type < PM_DEAD )
	{
		// use predicted values
		for ( i = 0; i < 3; i++ )
		{
			cl.refdef.viewangles[i] = cl.predAngles[i];
		}
	}
	else
	{
		// just use interpolated values
		for ( i = 0; i < 3; i++ )
		{
			cl.refdef.viewangles[i] = LerpAngle( ops->viewangles[i], ps->viewangles[i], lerp );
		}
	}

	for ( i = 0; i < 3; i++ )
	{
		cl.refdef.viewangles[i] += LerpAngle( ops->kick_angles[i], ps->kick_angles[i], lerp );
	}

	cl.refdef.viewangles[2] += CL_CalcRoll( cl.refdef.viewangles, cl.predMove.velocity );

	AngleVectors( cl.refdef.viewangles, cl.v_forward, cl.v_right, cl.v_up );

	// interpolate field of view
	cl.refdef.fov_x = ops->fov + lerp * ( ps->fov - ops->fov );

	// don't interpolate blend color
	for ( i = 0; i < 4; i++ )
	{
		cl.refdef.blend[i] = ps->blend[i];
	}

	//SV_CalcGunOffset( ps, ops );

	// add the weapon
	CL_AddViewWeapon( ps, ops, bob );
}

/*
========================
CL_AddEntities

Emits all entities, particles, and lights to the refresh
========================
*/
void CL_AddEntities()
{
	if ( cls.state != ca_active ) {
		return;
	}

	if ( cl.time > cl.frame.servertime )
	{
		if ( cl_showclamp->GetBool() ) {
			Com_Printf( "high clamp %i\n", cl.time - cl.frame.servertime );
		}
		cl.time = cl.frame.servertime;
		cl.lerpfrac = 1.0f;
	}
	else if ( cl.time < cl.frame.servertime - 100 )
	{
		if ( cl_showclamp->GetBool() ) {
			Com_Printf( "low clamp %i\n", cl.frame.servertime - 100 - cl.time );
		}
		cl.time = cl.frame.servertime - 100;
		cl.lerpfrac = 0.0f;
	}
	else
	{
		cl.lerpfrac = 1.0f - ( cl.frame.servertime - cl.time ) * 0.01f;
	}

	if ( cl_timedemo->GetBool() ) {
		cl.lerpfrac = 1.0f;
	}

//	CL_AddPacketEntities (&cl.frame);
//	CL_AddTEnts ();
//	CL_AddParticles ();
//	CL_AddDLights ();
//	CL_AddLightStyles ();

	CL_CalcViewValues();
	// PMM - moved this here so the heat beam has the right values for the vieworg, and can lock the beam to the gun
	CL_AddPacketEntities( &cl.frame );

	cge->AddEntities();
}

/*
========================
CL_GetEntitySoundOrigin

Called to get the sound spatialization origin
========================
*/
void CL_GetEntitySoundOrigin( int ent, vec3_t org )
{
	centity_t *old;

	if ( ent < 0 || ent >= MAX_EDICTS ) {
		Com_Error( "CL_GetEntitySoundOrigin: bad ent" );
	}

	old = &cl_entities[ent];
	VectorCopy( old->lerp_origin, org );

	// FIXME: bmodel issues...
}
