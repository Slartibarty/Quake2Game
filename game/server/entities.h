
#pragma once

// Forward decl
class CBaseEntity;

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

extern std::vector<CBaseEntity *> g_entityList;

void Ent_RunSpawn( CBaseEntity *pEntity );
void Ent_RunThink( CBaseEntity *pEntity );

// Allocates an entity, adds it to the list, but does not spawn it
CBaseEntity *Ent_CreateEntityByName( const char *pClassName );

CBaseEntity *Ent_GetEntity( int i );
int Ent_GetNumEntities();

CBaseEntity *Ent_FindInRadius( CBaseEntity *from, vec3_t org, float rad );

void SpawnEntities( const char *mapname, char *entities, const char *spawnpoint );

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
