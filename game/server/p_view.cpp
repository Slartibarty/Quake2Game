
#include "g_local.h"

/*
========================
ClientEndServerFrame

Called for each player at the end of the server frame
and right after spawning
========================
*/
void ClientEndServerFrame( CBaseEntity *pEnt )
{
	CBasePlayer *pPlayer = reinterpret_cast<CBasePlayer *>( pEnt );

	// determine the view offsets
	VectorClear( pPlayer->m_client.ps.viewoffset );
	pPlayer->m_client.ps.viewoffset[2] = pPlayer->m_edict.viewheight;

	VectorCopy( pPlayer->m_edict.velocity, pPlayer->m_client.oldvelocity );
	VectorCopy( pPlayer->m_client.ps.viewangles, pPlayer->m_client.oldviewangles );

	// clear weapon kicks
	VectorClear( pPlayer->m_client.kick_origin );
	VectorClear( pPlayer->m_client.kick_angles );
}
