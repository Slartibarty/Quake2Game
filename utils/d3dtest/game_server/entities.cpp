/*
===================================================================================================

	Entities

===================================================================================================
*/

#include "svg_local.h"

std::vector<CBaseEntity *> g_entityList;

CEntityFactoryList *EntityFactoryList()
{
	static CEntityFactoryList entityFactoryList;
	return &entityFactoryList;
}

void Ent_RunSpawn( CBaseEntity *pEntity )
{
	pEntity->Spawn();
}

void Ent_RunThink( CBaseEntity *pEntity )
{
	pEntity->Think();
}

CBaseEntity *Ent_CreateEntityByName( const char *pClassName )
{
	// allocate it
	CBaseEntity *pEntity = EntityFactoryList()->CreateEntityByName( pClassName );
	if ( !pEntity )
	{
		Com_Printf( "Tried to spawn entity \"%s\", which does not exist!\n", pClassName );
		return nullptr;
	}

	// add it to the entity list
	g_entityList.push_back( pEntity );

	return pEntity;
}

CBaseEntity *Ent_GetEntity( entIndex_t i )
{
	return g_entityList[i];
}

entIndex_t Ent_GetNumEntities()
{
	return static_cast<int>( g_entityList.size() );
}
