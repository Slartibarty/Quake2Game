/*
===================================================================================================

	QSMF
	23/04/2021
	Slartibarty

	Builds an SMF (Static Mesh File) from an OBJ

	FBX NOTES:
	The HL2 pistol mesh uses two materials, but there is a rogue polygon somewhere. I'm not
	gonna write the code to deal with that right now, it requires re-ordering the vertex and index
	buffers

	TODO:
	Re-organise the vertex array, check for each index, insert it into the list, then go again,
	should eventually have a fully sorted vertex array, just set up meshes there, it'll be easier

===================================================================================================
*/

// core
#include "../../core/core.h"
#include "../../common/q_formats.h"

// utils core
#include "cmdlib.h"

// c++
#include <vector>
#include <string>
#include <unordered_map>

// external libs
#include "fbxsdk.h"
#include "meshoptimizer.h"

// local
#include "fbxutils.h"

struct fatVertex_t
{
	vec3 pos;
	vec2 st;
	vec3 normal;
	vec3 tangent;
	int materialID;
};

// stores persistent information about the data a FbxMesh contributed to this SMF
struct meshContribution_t
{
	std::vector<fatVertex_t> vertices;
};

using mesh_t = fmtSMF::mesh_t;
using vertex_t = fmtSMF::vertex_t;
using index_t = fmtSMF::index_t;

struct options_t
{
	char srcName[MAX_OSPATH];
	char smfName[MAX_OSPATH];
};

static options_t g_options;

static void PrintUsage()
{
	Com_Print( "Usage: qsmf [options] input.obj output.smf\n" );
}

static void PrintHelp()
{
	Com_Print(
		"Help:\n"
		"  -verbose             : If present, this parameter enables verbose (debug) printing\n"
	);
}

// parses the command line and spits out results into g_options
static bool ParseCommandLine( int argc, char **argv )
{
	if ( argc < 3 )
	{
		// need at least input and output
		PrintUsage();
		PrintHelp();
		return false;
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
		return false;
	}
	fclose( outFile );

	return true;
}

static void WriteSMF(
	std::vector<mesh_t> &rawMeshes,
	std::vector<vertex_t> &rawVertices,
	std::vector<index_t> &rawIndices )
{
	FILE *handle = fopen( g_options.smfName, "wb" );
	if ( !handle )
	{
		// should never happen
		return;
	}

	fmtSMF::header_t header;

	uint32 depth = sizeof( header );

	header.fourCC = fmtSMF::fourCC;
	header.version = fmtSMF::version;
	header.numMeshes = static_cast<uint32>( rawMeshes.size() );
	header.offsetMeshes = depth;
	depth += static_cast<uint32>( rawMeshes.size() * sizeof( mesh_t ) );
	header.numVerts = static_cast<uint32>( rawVertices.size() );
	header.offsetVerts = depth;
	depth += static_cast<uint32>( rawVertices.size() * sizeof( vertex_t ) );
	header.numIndices = static_cast<uint32>( rawIndices.size() );
	header.offsetIndices = depth;

	assert( header.numIndices % 3 == 0 );

	Com_Printf( "Model stats: %u verts, %u faces, %u indices\n", header.numVerts, header.numIndices / 3, header.numIndices );

	fwrite( &header, sizeof( header ), 1, handle );
	fwrite( rawMeshes.data(), rawMeshes.size() * sizeof( mesh_t ), 1, handle );
	fwrite( rawVertices.data(), rawVertices.size() * sizeof( vertex_t ), 1, handle );
	fwrite( rawIndices.data(), rawIndices.size() * sizeof( index_t ), 1, handle );
	fclose( handle );

	Com_Printf( "Successfully wrote %s\n", g_options.smfName );
}

static void SetupMeshes( FbxMesh *pMesh,
	std::vector<mesh_t> &rawMeshes,
	std::vector<vertex_t> &rawVertices,
	std::vector<index_t> &rawIndices )
{
	FbxGeometryElementMaterial *pMaterial = pMesh->GetElementMaterial( 0 );

	const int triangleCount = static_cast<int>( rawIndices.size() / 3 );

	int indexIter = 0, triIter = 0;
	int lastID = -1;
	int lastMeshIndex = -1;
	int lastOffset = 0;
	for ( ; indexIter < rawIndices.size(); indexIter += 3, ++triIter )
	{
		int id = pMaterial->GetIndexArray().GetAt( triIter );

		if ( id != lastID )
		{
			// new mesh
			mesh_t &mesh = rawMeshes.emplace_back();

			FbxSurfaceMaterial *pLocalMat = pMesh->GetNode()->GetMaterial( id );

			assert( pLocalMat );

			// *trollface*
			const char *materialName = pLocalMat->GetName();

			Q_strcpy_s( mesh.materialName, materialName );

			mesh.offsetIndices = static_cast<uint32>( indexIter );
			mesh.countIndices = 0;

			if ( lastMeshIndex != -1 )
			{
				rawMeshes[lastMeshIndex].countIndices = ( indexIter - lastOffset ) * 3;
			}

			lastID = id;
			lastMeshIndex = static_cast<uint32>( rawMeshes.size() - 1 );
			lastOffset = indexIter;
		}
	}

	if ( rawMeshes[lastMeshIndex].countIndices == 0 )
	{
		rawMeshes[lastMeshIndex].countIndices = static_cast<uint32>( rawIndices.size() - lastOffset );
	}
}

static int SortFatVertices( const void *p1, const void *p2 )
{
	const fatVertex_t *v1 = reinterpret_cast<const fatVertex_t *>( p1 );
	const fatVertex_t *v2 = reinterpret_cast<const fatVertex_t *>( p2 );

	if ( v1->materialID < v2->materialID )
	{
		return -1;
	}
	if ( v1->materialID == v2->materialID )
	{
		return 0;
	}
	if ( v1->materialID > v2->materialID )
	{
		return 1;
	}

	ASSUME( 0 );
	return 0;
}

static void AddMeshContribution( FbxMesh *pMesh, meshContribution_t &contribution, std::vector<std::string> &materialNames )
{
	constexpr int polygonSize = 3;

	const FbxNode *pNode = pMesh->GetNode();

	const int polygonCount = pMesh->GetPolygonCount();
	const FbxVector4 *pControlPoints = pMesh->GetControlPoints();

	const FbxGeometryElementMaterial *pMaterial = pMesh->GetElementMaterial( 0 );
	assert( pMaterial );

	contribution.vertices.reserve( polygonCount * polygonSize );

	int vertexID = 0;
	int lastMatID = -1;
	for ( int polyIter = 0; polyIter < polygonCount; ++polyIter )
	{
		for ( int vertIter = 0; vertIter < polygonSize; ++vertIter, ++vertexID )
		{
			int controlPointIndex = pMesh->GetPolygonVertex( polyIter, vertIter );
			if ( controlPointIndex < 0 )
			{
				Com_Print( "Warning: Found an invalid index\n" );
				continue;
			}

			fatVertex_t &vertex = contribution.vertices.emplace_back();

			const FbxVector4 controlPoint = pControlPoints[controlPointIndex];
			vertex.pos.x = static_cast<float>( controlPoint[0] );
			vertex.pos.y = static_cast<float>( controlPoint[1] );
			vertex.pos.z = static_cast<float>( controlPoint[2] );

			const FbxVector2 uv = FBX_GetUV( pMesh, polyIter, vertIter );
			vertex.st.x = static_cast<float>( uv[0] );
			vertex.st.y = static_cast<float>( uv[1] );

			const FbxVector4 normal = FBX_GetNormal( pMesh, vertexID );
			vertex.normal.x = static_cast<float>( normal[0] );
			vertex.normal.y = static_cast<float>( normal[1] );
			vertex.normal.z = static_cast<float>( normal[2] );

			const FbxVector4 tangent = FBX_GetTangent( pMesh, vertexID );
			vertex.tangent.x = static_cast<float>( tangent[0] );
			vertex.tangent.y = static_cast<float>( tangent[1] );
			vertex.tangent.z = static_cast<float>( tangent[2] );

			// find the name for this vertex, then match it to the array, if it doesn't exist, add it
			const int materialID = pMaterial->GetIndexArray().GetAt( vertexID );
			const FbxSurfaceMaterial *pLocalMat = pNode->GetMaterial( materialID );
			assert( pLocalMat );
			const char *pName = pLocalMat->GetName();

			int matchID = -1;
			for ( int i = 0; i < (int)materialNames.size(); ++i )
			{
				if ( Q_strcmp( pName, materialNames[i].c_str() ) == 0 )
				{
					// match found
					matchID = i;
					break;
				}
			}

			if ( matchID == -1 )
			{
				materialNames.push_back( pName );
				matchID = static_cast<int>( materialNames.size() - 1 );
			}

			vertex.materialID = matchID;
		}
	}

	// rawVertices is now full of complete vertex data

	//SetupMeshes( pMesh, rawMeshes, rawVertices, rawIndices );
}

#if 0
static void MeshifyTriSoup(
	std::vector<mesh_t> &outMeshes,
	std::vector<vertex_t> &outVertices,
	std::vector<index_t> &outIndices,
	std::vector<fatVertex_t> &inVertices,
	FbxMesh *pMesh )
{
	std::vector<mesh_t> tempMeshes;

	// set up meshes, we'll use these to merge identical vertices individually
	{
		uint i = 0;
		int32 lastID = -1;
		for ( ; i < (uint)inVertices.size(); ++i )
		{
			const fatVertex_t &fatVertex = inVertices[i];

			if ( fatVertex.materialID != lastID )
			{
				// new material
				mesh_t &mesh = tempMeshes.emplace_back();

				// *trollface*
				const char *materialName = pMesh->GetNode()->GetMaterial( fatVertex.materialID )->GetName();

				Q_strcpy_s( mesh.materialName, materialName );

				lastID = fatVertex.materialID;
			}
		}
	}
}
#endif

static int Operate()
{
	FbxManager *pManager = FbxManager::Create();
	FbxIOSettings *pIOSettings = FbxIOSettings::Create( pManager, IOSROOT );
	pManager->SetIOSettings( pIOSettings );

	FbxScene *pScene = FBX_ImportScene( pManager, g_options.srcName );
	if ( !pScene )
	{
		pManager->Destroy();
		return EXIT_FAILURE;
	}

	FbxNode *pRootNode = pScene->GetRootNode();
	if ( pRootNode )
	{
		int nodeCount = pRootNode->GetChildCount( true );

		Com_Printf( "Scene has %d node%s\n", nodeCount, nodeCount != 1 ? "s" : "" );

		// final vertices
		std::vector<std::string> materialNames;
		std::vector<meshContribution_t> contributions;

		for ( int i = 0; i < pRootNode->GetChildCount( false ); ++i )
		{
			FbxNode *pNode = pRootNode->GetChild( i );
			FbxMesh *pMesh = pNode->GetMesh();

			if ( !pMesh )
			{
				// not a mesh
				continue;
			}

			if ( !pMesh->IsTriangleMesh() )
			{
				Com_Printf( "%s is a non-triangulated mesh. You must export with triangulated meshes enabled!\n", pNode->GetName() );
				continue;
			}

			meshContribution_t &contribution = contributions.emplace_back();

			AddMeshContribution( pMesh, contribution, materialNames );
		}

		Com_Printf(
			"Collected trisoup contributions from %zu meshes\n"
			"Scene contains %zu active materials\n",
			contributions.size(), materialNames.size()
		);

		/*if ( verbose )
		{
			for ( uint i = 0; i < (uint)rawVertices.size(); ++i )
			{
				Com_Printf( "%u", rawVertices[i].materialID );
				// every 20 characters, print a newline
				if ( i % 80 == 0 && i != 0 )
				{
					Com_Print( "\n" );
				}
			}
			Com_Print( "\n" );
		}

		qsort( rawVertices.data(), rawVertices.size(), sizeof( fatVertex_t ), SortFatVertices );

		if ( verbose )
		{
			for ( uint i = 0; i < (uint)rawVertices.size(); ++i )
			{
				Com_Printf( "%u", rawVertices[i].materialID );
				// every 20 characters, print a newline
				if ( i % 80 == 0 && i != 0 )
				{
					Com_Print( "\n" );
				}
			}
			Com_Print( "\n" );
		}*/

		/*
		std::vector<mesh_t> outMeshes;
		std::vector<vertex_t> outVertices;
		std::vector<index_t> outIndices;

		MeshifyTriSoup( outMeshes, outVertices, outIndices, rawVertices, pFirstMesh );
		*/

		//WriteSMF( rawMeshes, rawVertices, rawIndices );
	}

	pManager->Destroy();

	return EXIT_SUCCESS;
}

int main( int argc, char **argv )
{
	Com_Print( "---- QSMF model compiler - By Slartibarty ----\n" );

	Time_Init();

	if ( !ParseCommandLine( argc, argv ) ) {
		return EXIT_FAILURE;
	}

	return Operate();
}
