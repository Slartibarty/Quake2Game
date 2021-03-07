
#include "../../common/q_shared.h"
#include "../../common/q_formats.h"

#include <vector>

#include "../../external/xatlas/xatlas.h"

std::vector<dplane_t>	g_planes;
std::vector<dvertex_t>	g_vertices;
std::vector<texinfo_t>	g_texinfos;
std::vector<dface_t>	g_faces;
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

#if 1

int XAtlas_PrintCallback( const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	printf( "%s", msg );

	return 0;
}

int main( int argc, char **argv )
{
	if ( argc < 2 )
	{
		return 0;
	}

	FILE *bspHandle = fopen( argv[1], "rb" );
	if ( !bspHandle )
	{
		return 0;
	}

	fseek( bspHandle, 0, SEEK_END );
	int bspLength = ftell( bspHandle );
	fseek( bspHandle, 0, SEEK_SET );

	byte *bspBuffer = (byte *)malloc( bspLength );

	if ( fread( bspBuffer, bspLength, 1, bspHandle ) != 1 )
	{
		fclose( bspHandle );
		return 0;
	}

	fclose( bspHandle );

	dheader_t *bspHeader = (dheader_t *)bspBuffer;

	LoadPlanes( bspBuffer, &bspHeader->lumps[LUMP_PLANES] );
	LoadVertices( bspBuffer, &bspHeader->lumps[LUMP_VERTEXES] );
	LoadTexinfo( bspBuffer, &bspHeader->lumps[LUMP_TEXINFO] );
	LoadFaces( bspBuffer, &bspHeader->lumps[LUMP_FACES] );
	LoadEdges( bspBuffer, &bspHeader->lumps[LUMP_EDGES] );
	LoadSurfedges( bspBuffer, &bspHeader->lumps[LUMP_SURFEDGES] );

	free( bspBuffer );

	// Cross ref every vertex
#if 1
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

	//	assert( face.numedges % 3 == 0 );

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
		printf( "XAtlas failed: %s\n", xatlas::StringForEnum( meshError ) );
		return 1;
	}

	xatlas::ChartOptions chartOptions
	{
	//	.useInputMeshUvs = true,
		.fixWinding = true
	};

	xatlas::PackOptions packOptions
	{
		.rotateCharts = false
	};

	xatlas::Generate( pAtlas/*, chartOptions, packOptions*/ );

	printf( "%d charts\n", pAtlas->chartCount );
	printf( "%d atlases\n", pAtlas->atlasCount );

	for ( uint32 i = 0; i < pAtlas->atlasCount; ++i )
		printf( "%u: %0.2f%% utilization\n", i, pAtlas->utilization[i] * 100.0f );
	printf( "%ux%u resolution\n", pAtlas->width, pAtlas->height );
#endif

	// For every face, and each vertex used by that face, create new vertices for it
	// we export the raw vertices in the OBJ, but we need these for texture coordinates

	FILE *objHandle = fopen( "data.obj", "wb" );
#if 0
	if ( objHandle )
	{
		// Write vertices, skip 0
		for ( size_t i = 1; i < g_vertices.size(); ++i )
		{
			vec3_t &vert = g_vertices[i].point;
			fprintf( objHandle, "v %g %g %g\n", vert[0], vert[1], vert[2] );
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

#if 0
		// For each face
		for ( size_t iter1 = 0; iter1 < g_faces.size(); ++iter1 )
		{
			dface_t &face = g_faces[iter1];

			int currentEdge = face.firstedge;

			fprintf( objHandle, "f " );

			// For each edge of this face
			for ( size_t iter2 = 0; iter2 < face.numedges; ++iter2, ++currentEdge )
			{
				int currentSurfedge = g_surfedges[currentEdge];
				int start = currentSurfedge > 0 ? 0 : 1;
				currentSurfedge = abs( currentSurfedge );
				dedge_t &edge = g_edges[currentSurfedge];

				fprintf( objHandle, "%d%c", edge.v[start], ( iter2 == face.numedges - 1 ) ? '\n' : ' ' );
			}
		}
#else
		for ( size_t i = 0; i < triangulated_indices.size(); i += 3 )
		{
			fprintf( objHandle, "f %d %d %d\n", triangulated_indices[i]+1, triangulated_indices[i+1]+1, triangulated_indices[i+2]+1 );
		}
#endif

		fclose( objHandle );
	}
#else
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
#endif
	
	return 0;
}

#else

int XAtlas_PrintCallback( const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	printf( "%s", msg );

	return 0;
}

int main( int argc, char **argv )
{
	if ( argc < 2 )
	{
		return 0;
	}

	FILE *bspHandle = fopen( argv[1], "rb" );
	if ( !bspHandle )
	{
		return 0;
	}

	fseek( bspHandle, 0, SEEK_END );
	int bspLength = ftell( bspHandle );
	fseek( bspHandle, 0, SEEK_SET );

	byte *bspBuffer = (byte *)malloc( bspLength );

	if ( fread( bspBuffer, bspLength, 1, bspHandle ) != 1 )
	{
		fclose( bspHandle );
		return 0;
	}

	fclose( bspHandle );

	dheader_t *bspHeader = (dheader_t *)bspBuffer;

	LoadPlanes( bspBuffer, &bspHeader->lumps[LUMP_PLANES] );
	LoadVertices( bspBuffer, &bspHeader->lumps[LUMP_VERTEXES] );
	LoadTexinfo( bspBuffer, &bspHeader->lumps[LUMP_TEXINFO] );
	LoadFaces( bspBuffer, &bspHeader->lumps[LUMP_FACES] );
	LoadEdges( bspBuffer, &bspHeader->lumps[LUMP_EDGES] );
	LoadSurfedges( bspBuffer, &bspHeader->lumps[LUMP_SURFEDGES] );

	free( bspBuffer );

	// Cross ref every vertex
#if 1
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

	std::vector<uint8> faceCountData;
	std::vector<uint32> faceIndexData;

	// For each face
	for ( size_t iter1 = 0; iter1 < g_faces.size(); ++iter1 )
	{
		dface_t &face = g_faces[iter1];

		faceCountData.push_back( (uint8)face.numedges );

		int currentEdge = face.firstedge;

		// For each edge of this face
		for ( size_t iter2 = 0; iter2 < face.numedges; ++iter2, ++currentEdge )
		{
			int currentSurfedge = g_surfedges[currentEdge];
			int start = currentSurfedge > 0 ? 0 : 1;
			currentSurfedge = abs( currentSurfedge );
			dedge_t &edge = g_edges[currentSurfedge];

			faceIndexData.push_back( edge.v[start] );
		}
	}

	xatlas::SetPrint( XAtlas_PrintCallback, true );
	xatlas::Atlas *pAtlas = xatlas::Create();

	const xatlas::MeshDecl meshDecl
	{
		.vertexPositionData = (void *)(g_vertices.data() + 1),
	//	.vertexUvData = (void *)( (float *)( polyData.data() ) + 3 ),
		.indexData = (void *)faceIndexData.data(),

		.faceVertexCount = faceCountData.data(),

		.vertexCount = (uint32)(g_vertices.size() - 1),
		.vertexPositionStride = sizeof( dvertex_t ),
	//	.vertexUvStride = sizeof( vertexSigCompact_t ),
		.indexCount = (uint32)faceIndexData.size(),
		.indexOffset = 0,
		.faceCount = (uint32)faceCountData.size()
	};

	xatlas::AddMeshError meshError = xatlas::AddMesh( pAtlas, meshDecl );
	if ( meshError != xatlas::AddMeshError::Success ) {
		xatlas::Destroy( pAtlas );
		printf( "XAtlas failed: %s\n", xatlas::StringForEnum( meshError ) );
		return 1;
	}

	xatlas::ChartOptions chartOptions
	{
	//	.useInputMeshUvs = true,
		.fixWinding = true
	};

	xatlas::PackOptions packOptions
	{
		.rotateCharts = false
	};

	xatlas::Generate( pAtlas, chartOptions, packOptions );

	printf( "%d charts\n", pAtlas->chartCount );
	printf( "%d atlases\n", pAtlas->atlasCount );

	for ( uint32 i = 0; i < pAtlas->atlasCount; ++i )
		printf( "%u: %0.2f%% utilization\n", i, pAtlas->utilization[i] * 100.0f );
	printf( "%ux%u resolution\n", pAtlas->width, pAtlas->height );

	FILE *objHandle = fopen( "data.obj", "wb" );
	if ( objHandle )
	{
		// Write vertices, skip 0
		for ( size_t i = 1; i < g_vertices.size(); ++i )
		{
			const xatlas::Vertex &vertex = pAtlas->meshes[0].vertexArray[i];

			vec3_t &vert = g_vertices[i].point;
			fprintf( objHandle, "v %g %g %g\n", vert[0], vert[1], vert[2] );
			fprintf( objHandle, "vt %g %g\n", vertex.uv[0] / pAtlas->width, 1.0f - ( vertex.uv[1] / pAtlas->height ) );
		}

		char *bspFilename = strrchr( argv[1], '/' ) + 1;
		if ( !( bspFilename - 1 ) )
		{
			bspFilename = strrchr( argv[1], '\\' ) + 1;
			if ( !( bspFilename - 1 ) )
			{
				bspFilename = nullptr;
			}
		}

		fprintf( objHandle, "o %s\n", bspFilename );
		fprintf( objHandle, "s off\n" );

		// For each face
		for ( size_t iter1 = 0; iter1 < g_faces.size(); ++iter1 )
		{
			dface_t &face = g_faces[iter1];

			int currentEdge = face.firstedge;

			fprintf( objHandle, "f " );

			// For each edge of this face
			for ( size_t iter2 = 0; iter2 < face.numedges; ++iter2, ++currentEdge )
			{
				int currentSurfedge = g_surfedges[currentEdge];
				int start = currentSurfedge > 0 ? 0 : 1;
				currentSurfedge = abs( currentSurfedge );
				dedge_t &edge = g_edges[currentSurfedge];

				fprintf( objHandle, "%d/%d%c", edge.v[start], edge.v[start],( iter2 == face.numedges - 1 ) ? '\n' : ' ' );
			}
		}

		fclose( objHandle );
	}

	return 0;
}

#endif