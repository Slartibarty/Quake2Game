
#include "g_local.h"


//==============================================================


/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
static void PutClientInServer( CBaseEntity *ent )
{
	vec3_t	mins = {-16, -16, -24};
	vec3_t	maxs = {16, 16, 32};
	int		index;
	vec3_t	spawn_origin{}, spawn_angles{};
	client_persistant_t	saved;
	client_respawn_t	resp;

	CBasePlayer *pPlayer = reinterpret_cast<CBasePlayer *>( ent );

	gclient_t *client = &pPlayer->m_client;

	VectorCopy (mins, pPlayer->m_edict.mins);
	VectorCopy (maxs, pPlayer->m_edict.maxs);

	// clear playerstate values
	memset (&pPlayer->m_edict.client->ps, 0, sizeof(client->ps));

	VectorCopy( spawn_origin, client->ps.pmove.origin );

	client->ps.fov = 106;

	client->ps.gunindex = 0;

	// clear entity state values
	pPlayer->m_edict.s.effects = 0;
	pPlayer->m_edict.s.modelindex = 255;		// will use the skin specified model
	pPlayer->m_edict.s.modelindex2 = 255;		// custom gun model
	// sknum is player num and weapon number
	// weapon number will be added in changeweapon
	pPlayer->m_edict.s.skinnum = 0;

	pPlayer->m_edict.s.frame = 0;
	VectorCopy (spawn_origin, pPlayer->m_edict.s.origin);
	pPlayer->m_edict.s.origin[2] += 1;	// make sure off ground
	VectorCopy (pPlayer->m_edict.s.origin, pPlayer->m_edict.s.old_origin);

	pPlayer->m_edict.s.angles[PITCH] = 0;
	pPlayer->m_edict.s.angles[YAW] = spawn_angles[YAW];
	pPlayer->m_edict.s.angles[ROLL] = 0;
	VectorCopy (pPlayer->m_edict.s.angles, client->ps.viewangles);

	gi.linkentity ( &pPlayer->m_edict );
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
void ClientBegin( IServerEntity *ent )
{
	CBasePlayer *pPlayer = reinterpret_cast<CBasePlayer *>( ent );

	// Find a free player to inhabit
	for ( int i = 0; i < game.maxclients; ++i )
	{
		CBasePlayer *pEmptyPlayer = reinterpret_cast<CBasePlayer *>( Ent_GetEntity( i + 1 ) );

	}
	PutClientInServer( ent );

	// send effect if in a multiplayer game
	if ( game.maxclients > 1 )
	{
		// blah
	}

	// make sure all view stuff is valid
	ClientEndServerFrame( ent );
}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.

The game can override any of the settings in place
(forcing skins or names, etc) before copying it off.
============
*/
void ClientUserinfoChanged (edict_t *ent, char *userinfo)
{
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
bool ClientConnect (edict_t *ent, char *userinfo)
{
	// they can connect
	ent->client = game.clients + (ent - g_edicts - 1);

	ClientUserinfoChanged (ent, userinfo);

	if ( game.maxclients > 1 )
	{
		// Bruh
	}

	ent->svflags = 0; // make sure we start with known default
	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect (edict_t *ent)
{
}

//==============================================================

// pmove doesn't need to know about passent and contentmask
static trace_t PM_trace( vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end )
{
	return gi.trace( start, mins, maxs, end, level.current_entity, MASK_PLAYERSOLID );
}

static void PM_PlaySound( const char *sample, float volume )
{
	gi.sound( level.current_entity, CHAN_AUTO, gi.soundindex( sample ), volume, ATTN_NORM, 0.0f );
}

static uint CheckBlock( void *b, int c )
{
	int	v = 0, i = 0;
	for ( ; i < c; i++ ) {
		v += ( (byte *)b )[i];
	}
	return v;
}

static void PrintPmove( pmove_t *pm )
{
	uint c1, c2;

	c1 = CheckBlock( &pm->s, sizeof( pm->s ) );
	c2 = CheckBlock( &pm->cmd, sizeof( pm->cmd ) );
	gi.dprintf( "sv %u %u\n", c1, c2 );
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void ClientThink (edict_t *ent, usercmd_t *ucmd)
{
	gclient_t	*client;
	edict_t	*other;
	int		i, j;
	pmove_t	pm;

	level.current_entity = ent;
	client = ent->client;

	// set up for pmove
	memset (&pm, 0, sizeof(pm));

	if (ent->movetype == MOVETYPE_NOCLIP)
		client->ps.pmove.pm_type = PM_NOCLIP;
	else if (ent->s.modelindex != 255)
		client->ps.pmove.pm_type = PM_GIB;
	else if (ent->deadflag)
		client->ps.pmove.pm_type = PM_DEAD;
	else
		client->ps.pmove.pm_type = PM_NORMAL;

	client->ps.pmove.gravity = sv_gravity->GetInt(); // TODO: RIP FLOATING POINT
	pm.s = client->ps.pmove;

	VectorCopy( ent->s.origin, pm.s.origin );
	VectorCopy( ent->velocity, pm.s.velocity );

	pm.cmd = *ucmd;

	pm.trace = PM_trace;	// adds default parms
	pm.pointcontents = gi.pointcontents;
	pm.playsound = PM_PlaySound;

	// perform a pmove
	PM_Simulate (&pm);

	// save results of pmove
	client->ps.pmove = pm.s;
	client->old_pmove = pm.s;

	VectorCopy( pm.s.origin, ent->s.origin );
	VectorCopy( pm.s.velocity, ent->velocity );

	VectorCopy (pm.mins, ent->mins);
	VectorCopy (pm.maxs, ent->maxs);

	VectorCopy( ucmd->angles, client->resp.cmd_angles );

	gi.linkentity (ent);
}

/*
========================
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
========================
*/
void ClientBeginServerFrame( CBaseEntity *ent )
{
}
