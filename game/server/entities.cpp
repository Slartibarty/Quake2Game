/*
===================================================================================================

	Entities

===================================================================================================
*/

#include "g_local.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

#include "entities/CBaseEntity.h"
#include "entities.h"

#define MyWriter PrettyWriter

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

CBaseEntity *Ent_GetEntity( int i )
{
	return g_entityList[i];
}

int Ent_GetNumEntities()
{
	return static_cast<int>( g_entityList.size() );
}

/*
========================
Ent_FindInRadius

Returns entities that have origins within a spherical area
========================
*/
CBaseEntity *Ent_FindInRadius( CBaseEntity *from, vec3_t org, float rad )
{
#ifdef IMPLEMENT_ME
	if ( !from )
	{
		from = g_entityList[0];
	}
	else
	{
		++from;
	}

	for ( ; from < &g_entityList[globals.num_edicts]; ++from )
	{
		vec3_t eorg;
		for ( int j = 0; j < 3; ++j )
		{
			eorg[j] = org[j] - ( from->m_edict.origin[j] + ( from->mins[j] + from->maxs[j] ) * 0.5f );
		}
		if ( VectorLength( eorg ) > rad )
		{
			continue;
		}
		return from;
	}
#endif

	return nullptr;
}

//=================================================================================================

static char *Ent_JsonifyEdict( char *data, rapidjson::MyWriter<rapidjson::StringBuffer> &writer )
{
	using namespace rapidjson;

	writer.StartObject();

	// Go through all the dictionary pairs
	while ( 1 )
	{
		// Parse the key
		char *com_token = COM_Parse( &data );
		if ( com_token[0] == '}' )
		{
			break;
		}
		if ( !data )
		{
			gi.error( "ED_ParseEntity: EOF without closing brace" );
		}

		writer.String( com_token );

		// Parse the value
		com_token = COM_Parse( &data );
		if ( !data )
		{
			gi.error( "ED_ParseEntity: EOF without closing brace" );
		}
		if ( com_token[0] == '}' )
		{
			gi.error( "ED_ParseEntity: closing brace without data" );
		}

		writer.String( com_token );
	}

	writer.EndObject();

	return data;
}

static void Ent_CreateJsonEdicts( const char *entsName, char *edicts )
{
	using namespace rapidjson;

	StringBuffer sb;
	MyWriter<StringBuffer> writer( sb );

	writer.StartArray();

	while ( 1 )
	{
		// Parse the opening brace	
		char *com_token = COM_Parse( &edicts );
		if ( !edicts )
		{
			break;
		}
		if ( com_token[0] != '{' )
		{
			Com_FatalErrorf( "Ent_CreateJsonEdicts: Found \"%s\" when expecting \"{\"", com_token );
		}

		edicts = Ent_JsonifyEdict( edicts, writer );
	}

	writer.EndArray();

	// Spit it out into a file beside the BSP, this is incredibly shitty
	fsHandle_t handle = gi.fileSystem->OpenFileWrite( entsName, FS_GAMEDIR );
	if ( handle )
	{
		gi.fileSystem->PrintFile( sb.GetString(), handle );
		gi.fileSystem->CloseFile( handle );
	}
	else
	{
		Com_Printf( "Ent_CreateJsonEdicts: Couldn't access \"%s\" for writing...", entsName );
	}
}

/*
========================
SpawnEntities

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
========================
*/
void SpawnEntities( const char *mapname, char *entities, const char *spawnpoint )
{
	using namespace rapidjson;

	memset( &level, 0, sizeof( level ) );

	Q_strcpy_s( level.mapname, mapname );
	Q_strcpy_s( game.spawnpoint, spawnpoint );

	const char *entsName = va( "maps/%s.ents", mapname );

	char *jsonScript;
	gi.fileSystem->LoadFile( entsName, (void **)&jsonScript, 1 );
	if ( !jsonScript )
	{
		// Create it
		Ent_CreateJsonEdicts( entsName, entities );
		// Try again
		gi.fileSystem->LoadFile( entsName, (void **)&jsonScript, 1 );
		if ( !jsonScript )
		{
			Com_Error( "Failed to make a json script out of the edicts, what the fuck.\n" );
		}
	}

	Document doc;
	doc.Parse( jsonScript );
	gi.fileSystem->FreeFile( jsonScript );
	if ( doc.HasParseError() || !doc.IsArray() )
	{
		Com_Errorf( "JSON parse error in %s at offset %zu\n", entsName, doc.GetErrorOffset() );
	}

	uint entityCount = 0;

	for ( Value::ConstValueIterator array = doc.Begin(); array != doc.End(); ++array )
	{
		Value::ConstMemberIterator classname = array->FindMember( "classname" );
		if ( classname != array->MemberEnd() && classname->value.IsString() )
		{
			CBaseEntity *pEntity = Ent_CreateEntityByName( classname->value.GetString() );
			if ( !pEntity )
			{
				continue;
			}

			for ( Value::ConstMemberIterator member = array->MemberBegin(); member != array->MemberEnd(); ++member )
			{
				if ( member == classname )
				{
					continue;
				}

				pEntity->KeyValue( member->name.GetString(), member->value.GetString() );
			}

			Ent_RunSpawn( pEntity );

			Com_Printf( "Spawning \"%s\"\n", classname->value.GetString() );

			++entityCount;
		}
	}

	Com_Printf( "Parsed and spawned %u entities\n", entityCount );
}
