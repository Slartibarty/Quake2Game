/*
===================================================================================================

	Game-Specific Server Logic

===================================================================================================
*/

#include "svg_local.h"

void SVG_Init()
{
	Com_Print( "Initialising Server-Game\n" );

	Ent_RunSpawn( Ent_CreateEntityByName( "point_printer" ) );
}

void SVG_Shutdown()
{
	Com_Print( "Shutting down Server-Game\n" );
}

void SVG_Frame( float deltaTime )
{
	// Give everything a chance to think
	for ( entIndex_t i = 0; i < Ent_GetNumEntities(); ++i )
	{
		CBaseEntity *pEntity = Ent_GetEntity( i );

		Ent_RunThink( pEntity );
	}

}
