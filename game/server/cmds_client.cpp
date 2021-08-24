/*
===================================================================================================

	Client commands

===================================================================================================
*/

#include "g_local.h"

static void Cmd_Noclip_f( CBasePlayer *ent )
{
#if 0
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
#endif
}

static void Cmd_Kill_f( CBasePlayer *ent )
{
#if 0
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	player_die( ent, ent, ent, 100000, vec3_origin );
#endif
}

static void Cmd_Spawn_f( CBasePlayer *ent )
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
void ClientCommand( CBasePlayer *ent )
{
	/*if ( !ent->m_client )
	{
		// not fully in game yet
		return;
	}*/

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

	gi.cprintf( &ent->m_edict, PRINT_HIGH, "Unknown command \"%s\"\n", cmdName );
}
