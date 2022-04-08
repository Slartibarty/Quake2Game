// cl.input.c  -- builds an intended movement command to send to the server

#include "cl_local.h"

static StaticCvar cl_nodelta( "cl_nodelta", "0", 0 );

static uint old_sys_frameTime;
static uint frame_msec;

/*
===================================================================================================

	Key Buttons

	Continuous button event tracking is complicated by the fact that two different
	input sources (say, mouse button 1 and the control key) can both press the
	same button, but the button should only be released when both of the
	pressing key have been released.

	When a key event issues a button command (+forward, +attack, etc), it appends
	its key number as a parameter to the command so it can be matched up with
	the release.

	state bit 0 is the current state of the key
	state bit 1 is edge triggered on the up to down transition
	state bit 2 is edge triggered on the down to up transition


	Key_Event (int key, qboolean down, unsigned time);

	  +mlook src time

===================================================================================================
*/

static kbutton_t	in_left, in_right, in_forward, in_back;
static kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
static kbutton_t	in_speed, in_use, in_attack;
static kbutton_t	in_up, in_down;

static void IN_CenterView()
{
	cl.viewangles[PITCH] = -cl.frame.playerstate.pmove.delta_angles[PITCH];
}

static void IN_KeyDown( kbutton_t *b )
{
	int			k;
	const char	*c;

	c = Cmd_Argv( 1 );
	if ( c[0] ) {
		k = Q_atoi( c );
	}
	else {
		// typed manually at the console for continuous down
		k = -1;
	}

	if ( k == b->down[0] || k == b->down[1] ) {
		// repeating key
		return;
	}

	if ( !b->down[0] ) {
		b->down[0] = k;
	}
	else if ( !b->down[1] ) {
		b->down[1] = k;
	}
	else {
		Com_Print( "Three keys down for a button!\n" );
		return;
	}

	if ( b->state & 1 ) {
		// still down
		return;
	}

	// save timestamp
	c = Cmd_Argv( 2 );
	b->downtime = Q_atoui( c );
	if ( !b->downtime ) {
		b->downtime = sys_frameTime - 100;
	}

	b->state |= 1 + 2;	// down + impulse down
}

static void IN_KeyUp( kbutton_t *b )
{
	int			k;
	const char	*c;
	uint		uptime;

	c = Cmd_Argv( 1 );
	if ( c[0] ) {
		k = Q_atoi( c );
	}
	else {
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	if ( b->down[0] == k ) {
		b->down[0] = 0;
	}
	else if ( b->down[1] == k ) {
		b->down[1] = 0;
	}
	else {
		// key up without coresponding down (menu pass through)
		return;
	}
	if ( b->down[0] || b->down[1] ) {
		// some other key is still holding it down
		return;
	}

	if ( !( b->state & 1 ) ) {
		// still up (this should not happen)
		return;
	}

	// save timestamp
	c = Cmd_Argv( 2 );
	uptime = Q_atoui( c );
	if ( uptime ) {
		b->msec += uptime - b->downtime;
	}
	else {
		b->msec += 10;
	}

	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

static void IN_UpDown()			{ IN_KeyDown( &in_up ); }
static void IN_UpUp()			{ IN_KeyUp( &in_up ); }
static void IN_DownDown()		{ IN_KeyDown( &in_down ); }
static void IN_DownUp()			{ IN_KeyUp( &in_down ); }
static void IN_LeftDown()		{ IN_KeyDown( &in_left ); }
static void IN_LeftUp()			{ IN_KeyUp( &in_left ); }
static void IN_RightDown()		{ IN_KeyDown( &in_right ); }
static void IN_RightUp()		{ IN_KeyUp( &in_right ); }
static void IN_ForwardDown()	{ IN_KeyDown( &in_forward ); }
static void IN_ForwardUp()		{ IN_KeyUp( &in_forward ); }
static void IN_BackDown()		{ IN_KeyDown( &in_back ); }
static void IN_BackUp()			{ IN_KeyUp( &in_back ); }
static void IN_LookupDown()		{ IN_KeyDown( &in_lookup ); }
static void IN_LookupUp()		{ IN_KeyUp( &in_lookup ); }
static void IN_LookdownDown()	{ IN_KeyDown( &in_lookdown ); }
static void IN_LookdownUp()		{ IN_KeyUp( &in_lookdown ); }
static void IN_MoveleftDown()	{ IN_KeyDown( &in_moveleft ); }
static void IN_MoveleftUp()		{ IN_KeyUp( &in_moveleft ); }
static void IN_MoverightDown()	{ IN_KeyDown( &in_moveright ); }
static void IN_MoverightUp()	{ IN_KeyUp( &in_moveright ); }

static void IN_SpeedDown()		{ IN_KeyDown( &in_speed ); }
static void IN_SpeedUp()		{ IN_KeyUp( &in_speed ); }

static void IN_AttackDown()		{ IN_KeyDown( &in_attack ); }
static void IN_AttackUp()		{ IN_KeyUp( &in_attack ); }

static void IN_UseDown()		{ IN_KeyDown( &in_use ); }
static void IN_UseUp()			{ IN_KeyUp( &in_use ); }

/*
========================
CL_KeyState

Returns the fraction of the frame that the key was down
========================
*/
float CL_KeyState( kbutton_t *key )
{
	float	val;
	int		msec;

	// clear impulses
	key->state &= 1;

	msec = key->msec;
	key->msec = 0;

	if ( key->state ) {
		// still down
		if ( !key->downtime ) {
			msec = sys_frameTime;
		}
		else {
			msec += sys_frameTime - key->downtime;
		}
		key->downtime = sys_frameTime;
	}

	val = Clamp( (float)msec / frame_msec, 0.0f, 1.0f );

	return val;
}

//=================================================================================================

/*
========================
CL_AdjustAngles

Moves the local angle positions
========================
*/
static void CL_AdjustAngles()
{
	float speed;

	if ( in_speed.state & 1 ) {
		speed = cls.frametime * cl_anglespeedkey.GetFloat();
	} else {
		speed = cls.frametime;
	}

	cl.viewangles[PITCH] -= speed * cl_pitchspeed.GetFloat() * CL_KeyState( &in_lookup );
	cl.viewangles[PITCH] += speed * cl_pitchspeed.GetFloat() * CL_KeyState( &in_lookdown );
}

/*
========================
CL_BaseMove
========================
*/
static void CL_BaseMove( usercmd_t &cmd )
{
	CL_AdjustAngles();

	VectorCopy( cl.viewangles, cmd.angles );

	cmd.forwardmove += cl_forwardspeed.GetFloat() * CL_KeyState( &in_forward );
	cmd.forwardmove -= cl_forwardspeed.GetFloat() * CL_KeyState( &in_back );

	cmd.sidemove += cl_sidespeed.GetFloat() * CL_KeyState( &in_moveright );
	cmd.sidemove -= cl_sidespeed.GetFloat() * CL_KeyState( &in_moveleft );

	cmd.upmove += cl_upspeed.GetFloat() * CL_KeyState( &in_up );
	cmd.upmove -= cl_upspeed.GetFloat() * CL_KeyState( &in_down );

	//
	// adjust for speed key / running
	//
	if ( ( in_speed.state & 1 ) ^ (int)( cl_run.GetBool() ) )
	{
		cmd.forwardmove *= 2;
		cmd.sidemove *= 2;
		cmd.upmove *= 2;
	}
}

/*
========================
CL_ClampPitch
========================
*/
static void CL_ClampPitch()
{
	float pitch = cl.frame.playerstate.pmove.delta_angles[PITCH];
	if ( pitch > 180 ) {
		pitch -= 360;
	}

	if ( cl.viewangles[PITCH] + pitch < -360 ) {
		// wrapped
		cl.viewangles[PITCH] += 360;
	}
	if ( cl.viewangles[PITCH] + pitch > 360 ) {
		// wrapped
		cl.viewangles[PITCH] -= 360;
	}

	if ( cl.viewangles[PITCH] + pitch > 89 ) {
		cl.viewangles[PITCH] = 89 - pitch;
	}
	if ( cl.viewangles[PITCH] + pitch < -89 ) {
		cl.viewangles[PITCH] = -89 - pitch;
	}
}

/*
========================
CL_FinishMove
========================
*/
static void CL_FinishMove( usercmd_t &cmd )
{
	//
	// figure button bits
	//
	if ( in_attack.state & 3 ) {
		cmd.buttons |= BUTTON_ATTACK;
	}
	in_attack.state &= ~2;

	if ( in_use.state & 3 ) {
		cmd.buttons |= BUTTON_USE;
	}
	in_use.state &= ~2;

	if ( anykeydown && cls.key_dest == key_game ) {
		cmd.buttons |= BUTTON_ANY;
	}

	// send milliseconds of time to apply the move
	int ms = static_cast<int>( SEC2MS( cls.frametime ) );
	if ( ms > 250 ) {
		// time was unreasonable
		ms = 100;
	}
	cmd.msec = ms;

	CL_ClampPitch();
	for ( int i = 0; i < 3; ++i ) {
		cmd.angles[i] = ANGLE2SHORT( cl.viewangles[i] );
	}

	// send the ambient light level at the player's current position
	cmd.lightlevel = (byte)cl_lightlevel.GetInt();
}

/*
========================
CL_CreateCmd
========================
*/
static usercmd_t CL_CreateCmd()
{
	usercmd_t cmd;

	frame_msec = Clamp<uint>( sys_frameTime - old_sys_frameTime, 1, 200 );

	memset( &cmd, 0, sizeof( cmd ) );

	// get basic movement from keyboard
	CL_BaseMove( cmd );

	// other movement stuff here...

	CL_FinishMove( cmd );

	old_sys_frameTime = sys_frameTime;

	return cmd;
}

/*
========================
CL_SendCmd
========================
*/
void CL_SendCmd()
{
	sizebuf_t	buf;
	byte		data[128];
	int			i;
	usercmd_t	*cmd, *oldcmd;
	usercmd_t	nullcmd;
	int			checksumIndex;

	// build a command even if not connected

	// save this command off for prediction
	i = cls.netchan.outgoing_sequence & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	cl.cmd_time[i] = cls.realtime;	// for netgraph ping calculation

	*cmd = CL_CreateCmd ();

	cl.cmd = *cmd;

	if (cls.state == ca_disconnected || cls.state == ca_connecting)
		return;

	if ( cls.state == ca_connected)
	{
		if (cls.netchan.message.cursize	|| curtime - cls.netchan.last_sent > 1000 )
			Netchan_Transmit (&cls.netchan, 0, NULL);
		return;
	}

	// send a userinfo update if needed
	if (userinfo_modified)
	{
		CL_FixUpGender();
		userinfo_modified = false;
		MSG_WriteByte (&cls.netchan.message, clc_userinfo);
		MSG_WriteString (&cls.netchan.message, Cvar_Userinfo() );
	}

	SZ_Init (&buf, data, sizeof(data));

	if (cmd->buttons && cl.cinematictime > 0 && !cl.attractloop 
		&& cls.realtime - cl.cinematictime > 1000)
	{	// skip the rest of the cinematic
		SCR_FinishCinematic ();
	}

	// begin a client move command
	MSG_WriteByte (&buf, clc_move);

	// save the position for a checksum byte
	checksumIndex = buf.cursize;
	MSG_WriteByte (&buf, 0);

	// let the server know what the last frame we
	// got was, so the next message can be delta compressed
	if (cl_nodelta.GetBool() || !cl.frame.valid || cls.demowaiting)
		MSG_WriteLong (&buf, -1);	// no compression
	else
		MSG_WriteLong (&buf, cl.frame.serverframe);

	// send this and the previous cmds in the message, so
	// if the last packet was dropped, it can be recovered
	i = (cls.netchan.outgoing_sequence-2) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	memset (&nullcmd, 0, sizeof(nullcmd));
	MSG_WriteDeltaUsercmd (&buf, &nullcmd, cmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence-1) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	MSG_WriteDeltaUsercmd (&buf, oldcmd, cmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	MSG_WriteDeltaUsercmd (&buf, oldcmd, cmd);

	// calculate a checksum over the move commands
	buf.data[checksumIndex] = COM_BlockSequenceCRCByte(
		buf.data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
		cls.netchan.outgoing_sequence);

	//
	// deliver the message
	//
	Netchan_Transmit (&cls.netchan, buf.cursize, buf.data);	
}

/*
========================
CL_InitInput
========================
*/
void CL_InitInput()
{
	Cmd_AddCommand( "centerview", IN_CenterView );

	Cmd_AddCommand( "+moveup", IN_UpDown );
	Cmd_AddCommand( "-moveup", IN_UpUp );
	Cmd_AddCommand( "+movedown", IN_DownDown );
	Cmd_AddCommand( "-movedown", IN_DownUp );
	Cmd_AddCommand( "+left", IN_LeftDown );
	Cmd_AddCommand( "-left", IN_LeftUp );
	Cmd_AddCommand( "+right", IN_RightDown );
	Cmd_AddCommand( "-right", IN_RightUp );
	Cmd_AddCommand( "+forward", IN_ForwardDown );
	Cmd_AddCommand( "-forward", IN_ForwardUp );
	Cmd_AddCommand( "+back", IN_BackDown );
	Cmd_AddCommand( "-back", IN_BackUp );
	Cmd_AddCommand( "+lookup", IN_LookupDown );
	Cmd_AddCommand( "-lookup", IN_LookupUp );
	Cmd_AddCommand( "+lookdown", IN_LookdownDown );
	Cmd_AddCommand( "-lookdown", IN_LookdownUp );
	Cmd_AddCommand( "+moveleft", IN_MoveleftDown );
	Cmd_AddCommand( "-moveleft", IN_MoveleftUp );
	Cmd_AddCommand( "+moveright", IN_MoverightDown );
	Cmd_AddCommand( "-moveright", IN_MoverightUp );
	Cmd_AddCommand( "+speed", IN_SpeedDown );
	Cmd_AddCommand( "-speed", IN_SpeedUp );
	Cmd_AddCommand( "+attack", IN_AttackDown );
	Cmd_AddCommand( "-attack", IN_AttackUp );
	Cmd_AddCommand( "+use", IN_UseDown );
	Cmd_AddCommand( "-use", IN_UseUp );
}
