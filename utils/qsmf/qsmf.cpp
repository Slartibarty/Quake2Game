/*
===================================================================================================

	QSMF
	23/04/2021
	Slartibarty

	Builds an SMF (Static Mesh File) from an OBJ

===================================================================================================
*/

#include "../../core/core.h"
#include "../../common/q_formats.h"

#include "cmdlib.h"

#include "fbxsdk.h"

#include "obj_reader.h"

struct options_t
{
	char srcName[MAX_OSPATH];
	char smfName[MAX_OSPATH];
};

static options_t g_options;

static void PrintUsage( const char *name )
{
	Com_Printf( "Usage: %s [options] input.obj output.smf\n", name );
}

static void PrintHelp()
{
	Com_Print(
		"Help:\n"
		"  -verbose             : If present, this parameter enables verbose (debug) printing\n"
	);
}

static void ParseCommandLine( int argc, char **argv )
{
	if ( argc < 3 )
	{
		// need at least input and output
		PrintUsage( argv[0] );
		PrintHelp();
		exit( EXIT_FAILURE );
	}

	Time_Init();

	Q_strcpy_s( g_options.srcName, argv[argc - 2] );
	Str_FixSlashes( g_options.srcName );
	Q_strcpy_s( g_options.smfName, argv[argc - 1] );
	Str_FixSlashes( g_options.smfName );

	int argIter;
	for ( argIter = 1; argIter < argc; ++argIter )
	{
		const char *token = argv[argIter];

		if ( Q_stricmp( token, "-verbose" ) == 0 )
		{
			verbose = true;
			continue;
		}
	}

	// prematurely open our output file so we are sure we can write to it
	// if compilation fails this means we're left with a 0kb file, but that's okay for now
	FILE *outFile = fopen( g_options.smfName, "wb" );
	if ( !outFile )
	{
		Com_Printf( "Couldn't open %s for writing\n", g_options.smfName );
		exit( EXIT_FAILURE );
	}
	fclose( outFile );
}

#if 0

static int s_numTabs = 0;

static void PrintTabs()
{
	for ( int i = 0; i < s_numTabs; i++ )
	{
		Com_Print( "\t" );
	}
}

static const char *GetAttributeTypeName( FbxNodeAttribute::EType type )
{
	switch ( type )
	{
	case FbxNodeAttribute::eUnknown: return "unidentified";
	case FbxNodeAttribute::eNull: return "null";
	case FbxNodeAttribute::eMarker: return "marker";
	case FbxNodeAttribute::eSkeleton: return "skeleton";
	case FbxNodeAttribute::eMesh: return "mesh";
	case FbxNodeAttribute::eNurbs: return "nurbs";
	case FbxNodeAttribute::ePatch: return "patch";
	case FbxNodeAttribute::eCamera: return "camera";
	case FbxNodeAttribute::eCameraStereo: return "stereo";
	case FbxNodeAttribute::eCameraSwitcher: return "camera switcher";
	case FbxNodeAttribute::eLight: return "light";
	case FbxNodeAttribute::eOpticalReference: return "optical reference";
	case FbxNodeAttribute::eOpticalMarker: return "marker";
	case FbxNodeAttribute::eNurbsCurve: return "nurbs curve";
	case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface";
	case FbxNodeAttribute::eBoundary: return "boundary";
	case FbxNodeAttribute::eNurbsSurface: return "nurbs surface";
	case FbxNodeAttribute::eShape: return "shape";
	case FbxNodeAttribute::eLODGroup: return "lodgroup";
	case FbxNodeAttribute::eSubDiv: return "subdiv";
	default: return "unknown";
	}
}

static void PrintAttribute( FbxNodeAttribute *pAttribute )
{
	if ( !pAttribute )
	{
		return;
	}

	PrintTabs();

	const char *pTypeName = GetAttributeTypeName( pAttribute->GetAttributeType() );
	const char *pAttrName = pAttribute->GetName();

	Com_Printf( "<attribute type='%s' name='%s'/>\n", pTypeName, pAttrName );
}

static void PrintNode( FbxNode *pNode )
{
	PrintTabs();
	const char *nodeName = pNode->GetName();
	FbxDouble3 translation = pNode->LclTranslation.Get();
	FbxDouble3 rotation = pNode->LclRotation.Get();
	FbxDouble3 scaling = pNode->LclScaling.Get();

	// Print the contents of the node.
	Com_Printf( "<node name='%s' translation='(%f, %f, %f)' rotation='(%f, %f, %f)' scaling='(%f, %f, %f)'>\n",
		nodeName,
		translation[0], translation[1], translation[2],
		rotation[0], rotation[1], rotation[2],
		scaling[0], scaling[1], scaling[2]
	);
	++s_numTabs;

	// Print the node's attributes.
	for ( int i = 0; i < pNode->GetNodeAttributeCount(); ++i )
	{
		PrintAttribute( pNode->GetNodeAttributeByIndex( i ) );
	}

	// Recursively print the children.
	for ( int i = 0; i < pNode->GetChildCount(); ++i )
	{
		PrintNode( pNode->GetChild( i ) );
	}

	--s_numTabs;
	PrintTabs();
	Com_Print( "</node>\n" );
}

#endif

static void OperateOnNode( FbxNode *pNode )
{
	bool isMesh = false;
	for ( int i = 0; i < pNode->GetNodeAttributeCount(); ++i )
	{
		FbxNodeAttribute *pAttr = pNode->GetNodeAttributeByIndex( i );

		if ( pAttr->GetAttributeType() == FbxNodeAttribute::eMesh )
		{
			isMesh = true;
			break;
		}
	}

	if ( !isMesh )
	{
		return;
	}

	FbxMesh *pMesh = pNode->GetMesh();

	if ( !pMesh->IsTriangleMesh() )
	{
		Com_Print( "Found a non-triangulated mesh... Ignoring\n" );
		return;
	}

}

static FbxScene *ImportFBXScene( FbxManager *pManager )
{
	FbxImporter *pImporter = FbxImporter::Create( pManager, "" );

	Com_Printf( "Importing scene: %s\n", g_options.srcName );

	if ( !pImporter->Initialize( g_options.srcName, -1, pManager->GetIOSettings() ) )
	{
		Com_Print( "FbxImporter::Initialize failed\n" );
		Com_Printf( "Error returned: %s\n", pImporter->GetStatus().GetErrorString() );
		return nullptr;
	}

	FbxScene *pScene = FbxScene::Create( pManager, "" );

	if ( pScene )
	{
		pImporter->Import( pScene );
	}
	pImporter->Destroy();

	return pScene;
}

static int Operate()
{
	FbxManager *pManager = FbxManager::Create();
	FbxIOSettings *pIOSettings = FbxIOSettings::Create( pManager, IOSROOT );
	pManager->SetIOSettings( pIOSettings );

	FbxScene *pScene = ImportFBXScene( pManager );
	if ( !pScene )
	{
		pManager->Destroy();
		return EXIT_FAILURE;
	}

	FbxNode *pRootNode = pScene->GetRootNode();
	if ( pRootNode )
	{
		for ( int i = 0; i < pRootNode->GetChildCount(); ++i )
		{
			//PrintNode( pRootNode->GetChild( i ) );
			OperateOnNode( pRootNode->GetChild( i ) );
		}
	}

	pManager->Destroy();

	return EXIT_SUCCESS;
}

int main( int argc, char **argv )
{
	Com_Print( "---- QSMF model compiler - By Slartibarty ----\n" );

	Com_Print( "Using FBX SDK " FBXSDK_VERSION_STRING "\n" );

	ParseCommandLine( argc, argv );

	return Operate();
}
