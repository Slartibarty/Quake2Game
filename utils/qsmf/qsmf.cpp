/*
===================================================================================================

	QSMF
	23/04/2021
	Slartibarty

	Builds an SMF (Static Mesh File) from an OBJ

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

// external libs
#include "fbxsdk.h"
#include "meshoptimizer.h"

// local
#include "fbxutils.h"

// comment out to disable meshoptimizer optimisations
#define USE_MESHOPT

#define IS_SKINNING ( std::is_same<t_vert, fmtJMDL::skinnedVertex_t>::value )

template< typename t_vert >
struct fatVertex_t
{
	t_vert v;
	int materialID;
};

using mesh_t = fmtJMDL::mesh_t;

template< typename t_vert >
struct nodeWork_t
{
	std::vector<std::string>			materialNames;
	std::vector<fatVertex_t<t_vert>>	verts;
	std::vector<fmtJMDL::bone_t>		bones;
};

static struct options_t
{
	char srcName[MAX_OSPATH];
	char smfName[MAX_OSPATH];

	bool skeleton;
} g_options;

// parses the command line and spits out results into g_options
static bool ParseCommandLine( int argc, char **argv )
{
	if ( argc < 3 )
	{
		// need at least input and output
		Com_Print(
			"Usage: qsmf [options] input.fbx output.jmdl\n"
			"Options:\n"
			"  -verbose      : Enables verbose logging\n"
			"  -skeleton     : Creates a jmdl with skeletons\n"
		);
		return false;
	}

	Q_strcpy_s( g_options.srcName, argv[argc - 2] );
	Str_FixSlashes( g_options.srcName );
	Q_strcpy_s( g_options.smfName, argv[argc - 1] );
	Str_FixSlashes( g_options.smfName );

	int argIter;
	for ( argIter = 1; argIter < argc - 2; ++argIter )
	{
		const char *token = argv[argIter];

		if ( Q_stricmp( token, "-verbose" ) == 0 )
		{
			verbose = true;
			continue;
		}
		if ( Q_stricmp( token, "-skeleton" ) == 0 )
		{
			g_options.skeleton = true;
			continue;
		}

		Com_Printf( "Unrecognised token: %s\n", token );
		return false;
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

template< typename t_vert >
static void WriteJaffaModel(
	std::vector<mesh_t> &	rawMeshes,
	std::vector<t_vert> &	rawVertices,
	std::vector<uint32> &	rawIndices,
	std::vector<fmtJMDL::bone_t> & rawBones,
	uint32					indexSize )
{
	FILE *handle = fopen( g_options.smfName, "wb" );
	if ( !handle )
	{
		// should never happen
		return;
	}

	// start index slimming

	void *pIndexData = rawIndices.data();

	std::vector<uint16> outIndices;

	// slim down the indices if needed
	if ( indexSize == sizeof( uint16 ) )
	{
		outIndices.resize( rawIndices.size() );

		// make the indices small
		for ( uint i = 0; i < (uint)rawIndices.size(); ++i )
		{
			outIndices[i] = static_cast<uint16>( rawIndices[i] );
		}

		pIndexData = outIndices.data();
	}

	// end index slimming

	const bool hasBones = !rawBones.empty();

	fmtJMDL::header_t header{};

	uint32 depth = sizeof( header );

	header.fourCC = fmtJMDL::fourCC;
	header.version = fmtJMDL::version;

	if ( indexSize == sizeof( uint32 ) )
	{
		header.flags |= fmtJMDL::eBigIndices;
	}

	header.numMeshes = static_cast<uint32>( rawMeshes.size() );
	header.offsetMeshes = depth;
	depth += static_cast<uint32>( rawMeshes.size() * sizeof( mesh_t ) );
	header.numVerts = static_cast<uint32>( rawVertices.size() );
	header.offsetVerts = depth;
	depth += static_cast<uint32>( rawVertices.size() * sizeof( t_vert ) );
	header.numIndices = static_cast<uint32>( rawIndices.size() );
	header.offsetIndices = depth;
	depth += static_cast<uint32>( rawIndices.size() * indexSize );
	if ( hasBones )
	{
		header.numBones = static_cast<uint32>( rawBones.size() );
		header.offsetBones = depth;
	}

	assert( header.numIndices % 3 == 0 );

	Com_Printf( "Model stats: %u verts, %u triangles, %u indices\n", header.numVerts, header.numIndices / 3, header.numIndices );

	fwrite( &header, sizeof( header ), 1, handle );
	fwrite( rawMeshes.data(), rawMeshes.size() * sizeof( mesh_t ), 1, handle );
	fwrite( rawVertices.data(), rawVertices.size() * sizeof( t_vert ), 1, handle );
	fwrite( pIndexData, rawIndices.size() * indexSize, 1, handle );
	if ( hasBones )
	{
		fwrite( rawBones.data(), rawBones.size() * sizeof( fmtJMDL::bone_t ), 1, handle );
	}
	fclose( handle );

	Com_Printf( "Successfully wrote %s\n", g_options.smfName );
}

static void AddSkeletonContribution( FbxNode *pNode, std::vector<fmtJMDL::bone_t> &bones )
{
	FbxSkeleton *pSkeleton = pNode->GetSkeleton();

	fmtJMDL::bone_t &bone = bones.emplace_back();

	Q_strcpy_s( bone.name, pNode->GetName() );
	Com_Printf( "Bone name: %s\n", bone.name );

	if ( pSkeleton->IsSkeletonRoot() )
	{
		bone.parentIndex = -1;
	}
	else
	{
		FbxNode *pParent = pNode->GetParent();
		assert( pParent );
		bone.parentIndex = -2;
		// TODO: WTF LOL SLOW! I AM LAZY AND THIS IS STUPID!!!
		// Use an intermediary bones structure
		for ( int32 i = 0; i < (int32)bones.size(); ++i )
		{
			if ( i == (int32)( bones.size() - 1 ) )
			{
				Com_FatalError( "Error: Catastrophic failure" );
			}
			if ( Q_strcmp( bones[i].name, pParent->GetName() ) == 0 )
			{
				bone.parentIndex = static_cast<int32>( i );
				break;
			}
		}
		assert( bone.parentIndex >= 0 );
	}

	FbxDouble3 translation = pNode->LclTranslation.Get();
	FbxDouble3 rotation = pNode->LclRotation.Get();

	for ( int i = 0; i < 3; ++i )
	{
		bone.pos.v[i] = static_cast<float>( translation[i] );
		bone.rot.v[i] = static_cast<float>( rotation[i] );
	}

	int blah = 0;
	blah = 1;
}

template< typename t_vert >
static void AddMeshContribution( FbxNode *pNode, std::vector<fatVertex_t<t_vert>> &contribution, std::vector<std::string> &materialNames )
{
	constexpr int polygonSize = 3;

	FbxMesh *pMesh = pNode->GetMesh();

	// make sure we have tangents
	if ( pMesh->GenerateTangentsData( 0, false, false ) )
	{
		Com_Printf( "Built tangents for %s\n", pNode->GetName() );
	}

	const int polygonCount = pMesh->GetPolygonCount();
	assert( polygonCount > 0 );
	const FbxVector4 *pControlPoints = pMesh->GetControlPoints();

	const FbxGeometryElementMaterial *pMaterial = pMesh->GetElementMaterial( 0 );
	assert( pMaterial );

	const FbxMatrix transformMatrix( pNode->LclTranslation.Get(), pNode->LclRotation.Get(), pNode->LclScaling.Get() );

	int vertexID = 0;
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

			fatVertex_t<t_vert> &vertex = contribution.emplace_back();

			FbxVector4 finalControlPoint = transformMatrix.MultNormalize( pControlPoints[controlPointIndex] );

			vertex.v.pos.x = static_cast<float>( finalControlPoint[0] );
			vertex.v.pos.y = static_cast<float>( finalControlPoint[1] );
			vertex.v.pos.z = static_cast<float>( finalControlPoint[2] );

			const FbxVector2 uv = FBX_GetUV( pMesh, polyIter, vertIter );
			vertex.v.st.x = static_cast<float>( uv[0] );
			vertex.v.st.y = static_cast<float>( 1.0 - uv[1] );

			const FbxVector4 normal = FBX_GetNormal( pMesh, vertexID );
			vertex.v.normal.x = static_cast<float>( normal[0] );
			vertex.v.normal.y = static_cast<float>( normal[1] );
			vertex.v.normal.z = static_cast<float>( normal[2] );

			const FbxVector4 tangent = FBX_GetTangent( pMesh, vertexID );
			vertex.v.tangent.x = static_cast<float>( tangent[0] );
			vertex.v.tangent.y = static_cast<float>( tangent[1] );
			vertex.v.tangent.z = static_cast<float>( tangent[2] );

			if constexpr ( IS_SKINNING )
			{

			}

			// find the name for this vertex, then match it to the array, if it doesn't exist, add it
			const int materialID = pMaterial->GetIndexArray().GetAt( polyIter );
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
}

template< typename t_vert >
static void HandleNode_r( FbxNode *pNode, nodeWork_t<t_vert> &contributions )
{
	if ( pNode->GetAllObjectFlags() & FbxNode::eHidden )
	{
		// disregard hidden objects (doesn't work?)
		// TODO: THIS IS NOT ACCEPTABLE FOR FINDING CHILD NODES!!!
		return;
	}

	const FbxNodeAttribute *pAttr = pNode->GetNodeAttribute();

	if ( !pAttr )
	{
		Com_Print( "Warning: Null node attribute!\n" );
		return;
	}

	FbxNodeAttribute::EType type = pAttr->GetAttributeType();

	switch ( type )
	{
	case FbxNodeAttribute::eMesh:
		AddMeshContribution<t_vert>( pNode, contributions.verts, contributions.materialNames );
		break;
	case FbxNodeAttribute::eSkeleton:
		// Only add if we're skinning
		if constexpr ( IS_SKINNING )
		{
			AddSkeletonContribution( pNode, contributions.bones );
		}
		break;
	}

	for ( int i = 0; i < pNode->GetChildCount( false ); ++i )
	{
		HandleNode_r<t_vert>( pNode->GetChild( i ), contributions );
	}
}

template< typename t_vert >
static void SortVertices(
	const std::vector<fatVertex_t<t_vert>> &contributions,
	const std::vector<std::string> &materialNames,
	std::vector<t_vert> &sortedVertices,
	std::vector<mesh_t> &ranges)
{
	const int materialCount = (int)materialNames.size();

	sortedVertices.reserve( contributions.size() );

	// sort vertices into the new array, by material ID
	for ( int matIter = 0; matIter < materialCount; ++matIter )
	{
		uint32 rangeStart = UINT32_MAX;

		for ( uint vertIter = 0; vertIter < (uint)contributions.size(); ++vertIter )
		{
			const fatVertex_t<t_vert> &fatVertex = contributions[vertIter];

			if ( fatVertex.materialID == matIter )
			{
				t_vert &vertex = sortedVertices.emplace_back();

				// stash off the first index
				if ( rangeStart == UINT32_MAX )
				{
					rangeStart = static_cast<uint32>( sortedVertices.size() - 1 );
				}

				vertex.pos = fatVertex.v.pos;
				vertex.st = fatVertex.v.st;
				vertex.normal = fatVertex.v.normal;
				vertex.tangent = fatVertex.v.tangent;
			}
		}

		uint32 rangeEnd = static_cast<uint32>( sortedVertices.size() );
		assert( rangeStart != rangeEnd );

		uint32 rangeCount = rangeEnd - rangeStart;
		assert( rangeCount >= 3 );

		mesh_t &range = ranges.emplace_back();

		Q_strcpy_s( range.materialName, materialNames[matIter].c_str() );
		range.offsetIndices = rangeStart;
		range.countIndices = rangeCount;
	}

	assert( sortedVertices.size() == contributions.size() );
}

//
// NOTE: this function is sloooooow in debug, it's the slowest part of the program
// due to the search through "vertices"
// 
// the input vertices used to be fat, now they're not, hence the name
//
template< typename t_vert >
static uint32 IndexMesh(
	const std::vector<t_vert> &fatVertices,
	std::vector<t_vert> &vertices,
	std::vector<uint32> &indices )
{
	vertices.reserve( fatVertices.size() );
	indices.reserve( fatVertices.size() ); // TODO: is it faster to do resize() then direct access?

	for ( uint fatIter = 0; fatIter < (uint)fatVertices.size(); ++fatIter )
	{
		const t_vert &fatVertex = fatVertices[fatIter];

		// create an index for this vertex
		uint32 index = UINT32_MAX;

		// check all vertices to see if we have already created a vertex that is
		// close enough to this one
		for ( uint vertIter = 0; vertIter < (uint)vertices.size(); ++vertIter )
		{
			t_vert &vertex = vertices[vertIter];

			if ( Vec3Compare( fatVertex.pos, vertex.pos ) &&
				 Vec2Compare( fatVertex.st, vertex.st ) &&
				 Vec3Compare( fatVertex.normal, vertex.normal ) )
			{
				// average the tangents
				vertex.tangent.Add( fatVertex.tangent );

				index = vertIter;
			}
		}

		if ( index == UINT32_MAX )
		{
			// new vertex
			t_vert &vertex = vertices.emplace_back();

			vertex.pos = fatVertex.pos;
			vertex.st = fatVertex.st;
			vertex.normal = fatVertex.normal;
			vertex.tangent = fatVertex.tangent;

			index = static_cast<uint32>( vertices.size() - 1 );
		}

		indices.push_back( index );
	}

	if ( vertices.size() >= UINT16_MAX )
	{
		return sizeof( uint32 );
	}

	return sizeof( uint16 );
}

template< typename t_vert >
static void HandleScene( FbxNode *pRootNode )
{
	const int nodeCount = pRootNode->GetChildCount( true );

	Com_Printf( "Scene has %d node%s\n", nodeCount, nodeCount != 1 ? "s" : "" );

	nodeWork_t<t_vert> contributions;

	for ( int i = 0; i < pRootNode->GetChildCount( false ); ++i )
	{
		HandleNode_r<t_vert>( pRootNode->GetChild( i ), contributions );
	}

	// ensure the materials have .mat appended to them
	for ( uint i = 0; i < (uint)contributions.materialNames.size(); ++i )
	{
		if ( !contributions.materialNames[i].ends_with( ".mat" ) )
		{
			contributions.materialNames[i].append( ".mat" );
		}
	}

	Com_Printf(
		"Scene contains %zu active materials\n",
		//"Collected trisoup contributions from %zu meshes\n",
		contributions.materialNames.size()//, contributions.size()
	);

	std::vector<t_vert> sortedVertices;
	std::vector<mesh_t> ranges;

	SortVertices<t_vert>( contributions.verts, contributions.materialNames, sortedVertices, ranges );

	std::vector<t_vert> outVertices;
	std::vector<uint32> tmpIndices;			// uint32 indices, so we can optimise with meshopt

	uint32 indexSize = IndexMesh<t_vert>( sortedVertices, outVertices, tmpIndices );

#ifdef USE_MESHOPT

	for ( uint i = 0; i < (uint)ranges.size(); ++i )
	{
		uint32 *pOffsetIndices = tmpIndices.data() + ranges[i].offsetIndices;

		meshopt_optimizeVertexCache(
			pOffsetIndices, pOffsetIndices,
			ranges[i].countIndices, outVertices.size() );

		// TODO: what do we gain from doing this?
		/*
		meshopt_optimizeOverdraw(
			pOffsetIndices, pOffsetIndices,
			ranges[i].countIndices, (const float *)outVertices.data(), outVertices.size(), sizeof( vertex_t ), 1.05f );
		*/
	}

	size_t numVertices = meshopt_optimizeVertexFetch(
		outVertices.data(), tmpIndices.data(),
		tmpIndices.size(), outVertices.data(), outVertices.size(), sizeof( t_vert ) );

	outVertices.resize( numVertices );

#endif

	// optimise the range offsets for GL, so we don't do a * sizeof(index_t) every draw, haha
	for ( uint i = 0; i < (uint)ranges.size(); ++i )
	{
		ranges[i].offsetIndices *= indexSize;
	}

	WriteJaffaModel<t_vert>( ranges, outVertices, tmpIndices, contributions.bones, indexSize );
}

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

	// pre-triangulate our meshes
	{
		FbxGeometryConverter geoConverter( pManager );

		NodeList nodeList;
		geoConverter.RemoveBadPolygonsFromMeshes( pScene, &nodeList );
		if ( nodeList.Size() != 0)
		{
			Com_Printf( "Removed bad polygons from the following %d nodes:\n", nodeList.Size() );
			for ( int i = 0; i < nodeList.Size(); ++i )
			{
				Com_Printf( "    %s\n", nodeList[i]->GetName() );
			}
		}

		if ( !geoConverter.Triangulate( pScene, true ) )
		{
			Com_Print( "Warning: failed to triangulate some meshes, there may be errors!\n" );
		}
	}

	FbxNode *pRootNode = pScene->GetRootNode();
	if ( pRootNode )
	{
		if ( g_options.skeleton )
		{
			HandleScene<fmtJMDL::skinnedVertex_t>( pRootNode );
		}
		else
		{
			HandleScene<fmtJMDL::staticVertex_t>( pRootNode );
		}
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
