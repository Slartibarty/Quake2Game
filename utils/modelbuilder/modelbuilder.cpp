/*
===================================================================================================

	ModelBuilder

	Utilising an OOP style for this program

===================================================================================================
*/

#include "modelbuilder_local.h"

#include "rapidjson/document.h"

class CModelBuilder
{
private:

	// Contains settings relevant to building a model
	struct modelBuild_t
	{
		char *scriptPath = nullptr;				// The path from the working directory to this script file

		std::vector<std::string> scenes;
		std::vector<std::string> animations;

		bool hasSkeleton = false;
	};

	std::vector<modelBuild_t> m_modelBuilds;

	bool ParseCommandLine( int argc, char **argv )
	{
		using namespace rapidjson;

		if ( argc < 2 )
		{
			return false;
		}

		Document doc;

		for ( int i = 1; i < argc; ++i )
		{
			const char *filename = argv[i];
			char *buffer;
			Local_LoadFile( filename, (byte **)&buffer, 1 );
			if ( !buffer )
			{
				Com_Printf( "Failed to load %s\n", filename );
				return false;
			}

			doc.Parse( buffer );
			free( buffer );
			if ( doc.HasParseError() || !doc.IsObject() )
			{
				Com_Printf( "JSON parse error in %s at offset %zu\n", filename, doc.GetErrorOffset() );
				return false;
			}

			modelBuild_t &mdlBuild = m_modelBuilds.emplace_back();

			// Fill out scriptPath
			mdlBuild.scriptPath = _strdup( filename );
			Str_FixSlashes( mdlBuild.scriptPath );
			Local_StripFilename( mdlBuild.scriptPath );

			Value::ConstMemberIterator member;

			member = doc.FindMember( "Scenes" );
			if ( member != doc.MemberEnd() && member->value.IsArray() )
			{
				Value::ConstArray arr = member->value.GetArray();
				for ( SizeType i = 0; i < arr.Size(); ++i )
				{
					mdlBuild.scenes.push_back( arr[i].GetString() );
				}				
			}
			
			member = doc.FindMember( "Animations" );
			if ( member != doc.MemberEnd() && member->value.IsArray() )
			{
				Value::ConstArray arr = member->value.GetArray();
				for ( SizeType i = 0; i < arr.Size(); ++i )
				{
					mdlBuild.animations.push_back( arr[i].GetString() );
				}				
			}

			member = doc.FindMember( "HasSkeleton" );
			if ( member != doc.MemberEnd() && member->value.IsBool() )
			{
				mdlBuild.hasSkeleton = member->value.GetBool();
			}
		}

		Com_Printf( "Building %d jmdls\n", argc - 1 );

		return true;
	}

	void DoWorkOnModel( const modelBuild_t &mdlBuild )
	{
		CModelBuildInstance *pBuildInstance = new CModelBuildInstance;



		delete pBuildInstance;
	}

public:

	int Operate( int argc, char **argv )
	{
		Com_Print( "---- JaffaQuake Model Compiler - By Slartibarty ----\n\n" );

		Cvar_Init();
		Cvar_AddEarlyCommands( argc, argv );

		FileSystem::Init();

		if ( !ParseCommandLine( argc, argv ) )
		{
			return EXIT_FAILURE;
		}

		assert( m_modelBuilds.size() >= 1 );

		// Distribute work
		for ( uint32 i = 0; i < static_cast<uint32>( m_modelBuilds.size() ); ++i )
		{
			DoWorkOnModel( m_modelBuilds[i] );
		}

		return EXIT_SUCCESS;
	}

};

int main( int argc, char **argv )
{
	int returnValue;

	CModelBuilder *pModelBuilder = new CModelBuilder;
	returnValue = pModelBuilder->Operate( argc, argv );
	delete pModelBuilder;

	return returnValue;
}
