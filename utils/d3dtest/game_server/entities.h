/*
===================================================================================================

	Entities

===================================================================================================
*/

#pragma once

// Forward decl
class CBaseEntity;

// Use this when referring to entities by an index
using entIndex_t = int32;

// The amount of entity pointers we reserve
#define RESERVE_ENTITIES 1024

abstract_class IEntityFactory
{
public:
	virtual CBaseEntity *Create() = 0;
	virtual const char *GetClassName() = 0;
	virtual size_t GetEntitySize() = 0;
};

class CEntityFactoryList
{
private:
	std::vector<IEntityFactory *> entityFactories;

public:
	CEntityFactoryList()
	{
		entityFactories.reserve( RESERVE_ENTITIES );
	}

	void InstallFactory( IEntityFactory *pFactory )
	{
		entityFactories.push_back( pFactory );
	}

	// Allocates an entity by name
	CBaseEntity *CreateEntityByName( const char *pClassName )
	{
		for ( IEntityFactory *pFactory : entityFactories )
		{
			if ( Q_strcmp( pClassName, pFactory->GetClassName() ) == 0 )
			{
				return pFactory->Create();
			}
		}

		return nullptr;
	}
};

CEntityFactoryList *EntityFactoryList();

//=================================================================================================

// A list of all entities
extern std::vector<CBaseEntity *> g_entityList;

// Runs the spawn function for a given entity
void Ent_RunSpawn( CBaseEntity *pEntity );
// Runs the think function for a given entity
void Ent_RunThink( CBaseEntity *pEntity );

// Allocates an entity, adds it to the list, but does not spawn it
CBaseEntity *	Ent_CreateEntityByName( const char *pClassName );

// Gets a pointer to an entity by its index
CBaseEntity *	Ent_GetEntity( entIndex_t i );
// Returns the number of entities in the scene;
entIndex_t		Ent_GetNumEntities();

//=================================================================================================

template<typename T>
class CEntityFactory : public IEntityFactory
{
private:
	const char *m_pClassName;

public:
	CEntityFactory( const char *pClassName )
		: m_pClassName( pClassName )
	{
		EntityFactoryList()->InstallFactory( this );
	}

	CBaseEntity *Create() override
	{
		return new T;
	}

	const char *GetClassName() override
	{
		return m_pClassName;
	}

	size_t GetEntitySize() override
	{
		return sizeof( T );
	}
};

#define LINK_ENTITY_TO_CLASS(className, classDef) \
	static CEntityFactory<classDef> factory_##className(#className);

#define DECLARE_CLASS(thisClass, baseClass) using BaseClass = baseClass; using ThisClass = thisClass;		

#define DECLARE_CLASS_NOBASE(thisClass) using ThisClass = thisClass;
