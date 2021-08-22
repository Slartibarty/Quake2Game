/*
===================================================================================================

	Client commands

===================================================================================================
*/

#include "g_local.h"

static void Cmd_Noclip_f( edict_t *ent )
{
	const char *msg;

	if ( ent->movetype == MOVETYPE_NOCLIP )
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	gi.cprintf( ent, PRINT_HIGH, msg );
}

static void Cmd_Kill_f( edict_t *ent )
{
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	player_die( ent, ent, ent, 100000, vec3_origin );
}

static void Cmd_Spawn_f( edict_t *ent )
{
	if ( gi.argc() != 2 )
	{
		return;
	}

	const char *entName = gi.argv( 1 );

	CBaseEntity *pEntity = Ent_CreateEntityByName( entName );
	Ent_RunSpawn( pEntity );
}

//=================================================================================================

/*
========================
ClientCommand
========================
*/
void ClientCommand( edict_t *ent )
{
	if ( !ent->client )
	{
		// not fully in game yet
		return;
	}

	const char *cmdName = gi.argv( 0 );
	uint32 cmdHash = HashStringInsensitive( cmdName );

	switch ( cmdHash )
	{
	case ConstHashStringInsensitive( "noclip" ):
		Cmd_Noclip_f( ent );
		return;
	case ConstHashStringInsensitive( "kill" ):
		Cmd_Kill_f( ent );
		return;
	case ConstHashStringInsensitive( "spawn" ):
		Cmd_Spawn_f( ent );
		return;
	}

	gi.cprintf( ent, PRINT_HIGH, "Unknown command \"%s\"\n", cmdName );
}
