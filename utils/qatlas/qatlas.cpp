
#include "../../core/core.h"
#include "../../common/q_formats.h"

#include <vector>

#include "../../thirdparty/xatlas/xatlas.h"

std::vector<dplane_t>	g_planes;
std::vector<dvertex_t>	g_vertices;
std::vector<dnode_t>	g_nodes;
std::vector<texinfo_t>	g_texinfos;
std::vector<dface_t>	g_faces;
std::vector<dleaf_t>	g_leafs;
std::vector<uint16>		g_leafFaces;
std::vector<dedge_t>	g_edges;
std::vector<int>		g_surfedges;

static void LoadPlanes( byte *bspBuffer, lump_t *lump )
{
	int numPlanes = lump->filelen / sizeof( dplane_t );

	g_planes.reserve( numPlanes );

	dplane_t *planes = (dplane_t *)( bspBuffer + lump->fileofs );

	for ( int i = 0; i < numPlanes; ++i )
	{
		g_planes.push_back( planes[i] );
	}
}

static void LoadVertices( byte *bspBuffer, lump_t *lump )
{
	int numVertices = lump->filelen / sizeof( dvertex_t );

	g_vertices.reserve( numVertices );

	dvertex_t *vertices = (dvertex_t *)( bspBuffer + lump->fileofs );

	for ( int i = 0; i < numVertices; ++i )
	{
		g_vertices.push_back( vertices[i] );
	}
}

static void LoadNodes( byte *bspBuffer, lump_t *lump )
{
	int numNodes = lump->filelen / sizeof( dnode_t );

	g_nodes.reserve( numNodes );

	dnode_t *nodes = (dnode_t *)( bspBuffer + lump->fileofs );

	for ( int i = 0; i < numNodes; ++i )
	{
		g_nodes.push_back( nodes[i] );
	}
}

static void LoadFaces( byte *bspBuffer, lump_t *lump )
{
	int numFaces = lump->filelen / sizeof( dface_t );

	g_faces.reserve( numFaces );

	dface_t *faces = (dface_t *)( bspBuffer + lump->fileofs );

	for ( int i = 0; i < numFaces; ++i )
	{
		g_faces.push_back( faces[i] );
	}
}

static void LoadLeafs( byte *bspBuffer, lump_t *lump )
{
	int numLeafs = lump->filelen / sizeof( dleaf_t );

	g_leafs.reserve( numLeafs );

	dleaf_t *faces = (dleaf_t *)( bspBuffer + lump->fileofs );

	for ( int i = 0; i < numLeafs; ++i )
	{
		g_leafs.push_back( faces[i] );
	}
}

static void LoadLeafFaces( byte *bspBuffer, lump_t *lump )
{
	int numLeafFaces = lump->filelen / sizeof( uint16 );

	g_leafFaces.reserve( numLeafFaces );

	uint16 *faces = (uint16 *)( bspBuffer + lump->fileofs );

	for ( int i = 0; i < numLeafFaces; ++i )
	{
		g_leafFaces.push_back( faces[i] );
	}
}

static void LoadTexinfo( byte *bspBuffer, lump_t *lump )
{
	int numTexinfo = lump->filelen / sizeof( texinfo_t );

	g_texinfos.reserve( numTexinfo );

	texinfo_t *faces = (texinfo_t *)( bspBuffer + lump->fileofs );

	for ( int i = 0; i < numTexinfo; ++i )
	{
		g_texinfos.push_back( faces[i] );
	}
}

static void LoadEdges( byte *bspBuffer, lump_t *lump )
{
	int numEdges = lump->filelen / sizeof( dedge_t );

	g_edges.reserve( numEdges );

	dedge_t *edges = (dedge_t *)( bspBuffer + lump->fileofs );

	for ( int i = 0; i < numEdges; ++i )
	{
		g_edges.push_back( edges[i] );
	}
}

static void LoadSurfedges( byte *bspBuffer, lump_t *lump )
{
	int numSurfedges = lump->filelen / sizeof( int );

	g_surfedges.reserve( numSurfedges );

	int *surfedges = (int *)( bspBuffer + lump->fileofs );

	for ( int i = 0; i < numSurfedges; ++i )
	{
		g_surfedges.push_back( surfedges[i] );
	}
}

//=================================================================================================

struct bspTransition_t
{
	int oldIndex;
	int newIndex;
};

int XAtlas_PrintCallback( const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Print( msg );

	return 0;
}

int main( int argc, char **argv )
{
	Com_Print( "---- QATLAS - By Slartibarty ----\n" );

	if ( argc < 2 )
	{
		Com_Print( "Need more arguments\n" );
		return EXIT_FAILURE;
	}

	FILE *bspHandle = fopen( argv[1], "rb" );
	if ( !bspHandle )
	{
		Com_Printf( "Couldn't open %s\n", argv[1] );
		return EXIT_FAILURE;
	}

	fseek( bspHandle, 0, SEEK_END );
	int bspLength = ftell( bspHandle );
	fseek( bspHandle, 0, SEEK_SET );

	byte *bspBuffer = (byte *)malloc( bspLength );

	if ( fread( bspBuffer, bspLength, 1, bspHandle ) != 1 )
	{
		fclose( bspHandle );
		return EXIT_FAILURE;
	}

	fclose( bspHandle );

	dheader_t *bspHeader = (dheader_t *)bspBuffer;

	LoadPlanes( bspBuffer, &bspHeader->lumps[LUMP_PLANES] );
	LoadVertices( bspBuffer, &bspHeader->lumps[LUMP_VERTEXES] );
	LoadNodes( bspBuffer, &bspHeader->lumps[LUMP_NODES] );
	LoadTexinfo( bspBuffer, &bspHeader->lumps[LUMP_TEXINFO] );
	LoadFaces( bspBuffer, &bspHeader->lumps[LUMP_FACES] );
	LoadEdges( bspBuffer, &bspHeader->lumps[LUMP_EDGES] );
	LoadSurfedges( bspBuffer, &bspHeader->lumps[LUMP_SURFEDGES] );

	free( bspBuffer );

	// Cross ref every vertex
#if 0
	for ( size_t iter1 = 0; iter1 < g_vertices.size(); ++iter1 )
	{
		dvertex_t &vertex = g_vertices[iter1];

		for ( size_t iter2 = 0; iter2 < g_vertices.size(); ++iter2 )
		{
			if ( iter2 == iter1 )
			{
				continue;
			}

			dvertex_t &vertex2 = g_vertices[iter2];

			assert( VectorCompare( vertex.point, vertex2.point ) == false );
		}
	}
#endif

	std::vector<uint> triangulated_indices;
	std::vector<uint> face_indices;

	// for each node
	for ( size_t iter1 = 0; iter1 < g_nodes.size(); ++iter1, face_indices.clear() )
	{
		dnode_t node = g_nodes[iter1];

		//node.
	}

	// For each face
	for ( size_t iter1 = 0; iter1 < g_faces.size(); ++iter1, face_indices.clear() )
	{
		dface_t &face = g_faces[iter1];

		// face.numedges == number of vertices

		int currentEdge = face.firstedge;

		// For each edge of this face
		for ( size_t iter2 = 0; iter2 < face.numedges; ++iter2, ++currentEdge )
		{
			int currentSurfedge = g_surfedges[currentEdge];
			int start = currentSurfedge > 0 ? 0 : 1;
			currentSurfedge = abs( currentSurfedge );
			dedge_t &edge = g_edges[currentSurfedge];

			face_indices.push_back( edge.v[start] );
		}

		//assert( face.numedges % 3 == 0 );

		int numTriangles = face.numedges - 2;

		if ( face.numedges == 3 )
		{
			triangulated_indices.push_back( face_indices[0] );
			triangulated_indices.push_back( face_indices[1] );
			triangulated_indices.push_back( face_indices[2] );
			continue;
		}

		// Triangulate it

		int i;
		int middle;

		// For every triangle
		for ( i = 0, middle = 1; i < numTriangles; ++i, ++middle )
		{
			triangulated_indices.push_back( face_indices[0] );
			triangulated_indices.push_back( face_indices[middle] );
			triangulated_indices.push_back( face_indices[middle+1] );
		}

	}

#if 1
	xatlas::SetPrint( XAtlas_PrintCallback, true );
	xatlas::Atlas *pAtlas = xatlas::Create();

	const xatlas::MeshDecl meshDecl
	{
		.vertexPositionData = (void *)g_vertices.data(),
		.indexData = (void *)triangulated_indices.data(),

		.vertexCount = (uint32)g_vertices.size(),
		.vertexPositionStride = sizeof( dvertex_t ),
		.indexCount = (uint32)triangulated_indices.size(),
		.indexFormat = xatlas::IndexFormat::UInt32
	};

	xatlas::AddMeshError meshError = xatlas::AddMesh( pAtlas, meshDecl, 1 );
	if ( meshError != xatlas::AddMeshError::Success ) {
		xatlas::Destroy( pAtlas );
		Com_Printf( "XAtlas failed: %s\n", xatlas::StringForEnum( meshError ) );
		return 1;
	}

	xatlas::ChartOptions chartOptions
	{
		.maxIterations = 512
	};

	xatlas::PackOptions packOptions
	{
		.bruteForce = true
	};

	xatlas::Generate( pAtlas, chartOptions, packOptions );

	Com_Printf( "%d charts\n", pAtlas->chartCount );
	Com_Printf( "%d atlases\n", pAtlas->atlasCount );

	for ( uint32 i = 0; i < pAtlas->atlasCount; ++i )
		Com_Printf( "%u: %0.2f%% utilization\n", i, pAtlas->utilization[i] * 100.0f );
	Com_Printf( "%ux%u resolution\n", pAtlas->width, pAtlas->height );
#endif

	// For every face, and each vertex used by that face, create new vertices for it
	// we export the raw vertices in the OBJ, but we need these for texture coordinates

	FILE *objHandle = fopen( "data.obj", "wb" );
	if ( objHandle )
	{
		uint32 firstVertex = 0;
		for ( uint32 i = 0; i < pAtlas->meshCount; i++ )
		{
			const xatlas::Mesh &mesh = pAtlas->meshes[i];
			for ( uint32 v = 0; v < mesh.vertexCount; v++ )
			{
				const xatlas::Vertex &vertex = mesh.vertexArray[v];
				const float *pos = (float *)&g_vertices[vertex.xref];
				fprintf( objHandle, "v %g %g %g\n", pos[0], pos[1], pos[2] );
				//fprintf( objHandle, "vt %g %g\n", vertex.uv[0] / pAtlas->width, 1.0f - ( vertex.uv[1] / pAtlas->height ) );
				fprintf( objHandle, "vt %g %g\n", vertex.uv[0] / pAtlas->width, vertex.uv[1] / pAtlas->height );
			}

			const char *bspFilename = strrchr( argv[1], '/' ) + 1;
			if ( !( bspFilename - 1 ) )
			{
				bspFilename = strrchr( argv[1], '\\' ) + 1;
				if ( !( bspFilename - 1 ) )
				{
					bspFilename = "null";
				}
			}

			fprintf( objHandle, "o %s\n", bspFilename );
			fprintf( objHandle, "s off\n" );

			for ( uint32_t f = 0; f < mesh.indexCount; f += 3 )
			{
				const uint32_t index1 = firstVertex + mesh.indexArray[f] + 1; // 1-indexed
				const uint32_t index2 = firstVertex + mesh.indexArray[f+1] + 1; // 1-indexed
				const uint32_t index3 = firstVertex + mesh.indexArray[f+2] + 1; // 1-indexed
				fprintf( objHandle, "f %d/%d %d/%d %d/%d\n", index1, index1, index2, index2, index3, index3 );
			}

			firstVertex += mesh.vertexCount;
		}

		fclose( objHandle );
	}
	
	return 0;
}
