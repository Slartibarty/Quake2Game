/*
===================================================================================================

	PLAN:

	What: Decomposing the existing BSP data so that each node / leaf has an offset into a list of
	faces rather than the existing software-mode shit.

	Why: I want external lightmap UVs and to be able to use array-based drawing in the engine

	How: ...
	
	Lumps (output):
	BSP "face" lump, contains material names (TODO: stringtable) offsets into the index buffer,
	lots of duplicate data in here, no other easy way though

	BSP vertex lump, this is a collection of all unique vertices in the entire BSP, they're in the
	format XYZ ST

	BSP index lump, the face lump contains offsets into this, and a count


===================================================================================================
*/

#include "qatlas.h"

/*
RESULTS:
std::vector<bspDrawFace_t>		g_drawFaces;
std::vector<bspDrawVert_t>		g_drawVerts;
std::vector<bspDrawIndex_t>		g_drawIndices;
*/


struct richIndexData_t
{
	int32 texInfo;	// texinfo for this index
	uint index;
};

// fat-ass vert with a bunch of information alongside it
struct fatVert_t
{
	bspDrawVert_t drawVert;
	richIndexData_t rich;
	//uint originalIndex;		// index into g_vertices that this was sourced from
	//uint texInfo;				// texinfo that this was sourced from
};

struct texInfoMisc_t
{
	std::string name;
	std::vector<uint> texInfos;		// Texinfos associated with this material, for this node
};


void Decompose_Main()
{
	// the size of the global index buffer is added to this at the end of each node loop
	uint32 indexOffset = 0;
	uint32 indexOffset2 = 0;

	// loop 1: every node
	for ( uint iNode = 0; iNode < (uint)g_nodes.size(); ++iNode )
	{
		dnode_t &node = g_nodes[iNode];

		// add these to the global vectors at the end of the loop
		//std::vector<bspDrawFace_t> nodeDrawFaces;		// the draw faces for this node
		//std::vector<bspDrawVert_t> nodeDrawVerts;		// the draw verts for this node
		//std::vector<bspDrawIndex_t> nodeDrawIndices;	// the draw indices for this node

		std::vector<texInfoMisc_t> node_materials;		// names of materials used by this node

		std::vector<richIndexData_t> triangulated_indices;	// triangulated indices and their respective texinfos, index is into g_vertices

		// loop 2: node > every face
		for ( uint iFace = 0; iFace < node.numfaces; ++iFace )
		{
			const dface_t &face = g_faces[node.firstface + iFace];

			const texinfo_t faceTexInfo = g_texinfos[face.texinfo];

			if ( node_materials.size() == 0 )
			{
				// minor hack

				// texture not recorded, add it fresh
				auto &nmat = node_materials.emplace_back();

				nmat.name = faceTexInfo.texture;
				nmat.texInfos.push_back( face.texinfo );
			}
			else
			{
				bool wantNewMaterial = true;
				// loop 3: node > face > node materials
				for ( uint iMat = 0; iMat < (uint)node_materials.size(); ++iMat )
				{
					texInfoMisc_t &misc = node_materials[iMat];

					if ( Q_strcmp( misc.name.c_str(), faceTexInfo.texture ) == 0 )
					{
						// case: we have already recorded this texture, add a texinfo ref
						// if it doesn't already exist
						// loop 4: texinfo refs for this material
						bool foundRef = false;
						for ( uint iRefs = 0; iRefs < (uint)misc.texInfos.size(); ++iRefs )
						{
							if ( misc.texInfos[iRefs] == face.texinfo )
							{
								foundRef = true;
								break;
							}
						}
						if ( !foundRef )
						{
							// new texinfo found using this material name
							misc.texInfos.push_back( face.texinfo );
						}
						wantNewMaterial = false;
						break;
					}
				}

				if ( wantNewMaterial )
				{
					// texture not recorded, add it fresh
					auto &nmat = node_materials.emplace_back();

					nmat.name = faceTexInfo.texture;
					nmat.texInfos.push_back( face.texinfo );
				}
			}

			int currentEdge = face.firstedge;

			std::vector<richIndexData_t> face_indices;

			// loop 3: node > face > edge
			for ( int iEdge = 0; iEdge < face.numedges; ++iEdge, ++currentEdge )
			{
				int currentSurfedge = g_surfedges[currentEdge];
				int start = currentSurfedge > 0 ? 0 : 1;
				currentSurfedge = abs( currentSurfedge );
				dedge_t &edge = g_edges[currentSurfedge];

				face_indices.push_back( { face.texinfo, edge.v[start] } );
			}

			int numTriangles = face.numedges - 2;

			if ( face.numedges == 3 )
			{
				triangulated_indices.push_back( face_indices[0] );
				triangulated_indices.push_back( face_indices[1] );
				triangulated_indices.push_back( face_indices[2] );
				continue;
			}

			// triangulate it, fan style

			int i;
			int middle;

			// for every triangle
			for ( i = 0, middle = 1; i < numTriangles; ++i, ++middle )
			{
				triangulated_indices.push_back( face_indices[0] );
				triangulated_indices.push_back( face_indices[middle] );
				triangulated_indices.push_back( face_indices[middle + 1] );
			}
		}

		// these should be global
		std::vector<fatVert_t> fatVerts;

		std::vector<bspDrawFace_t> node_faces;
		std::vector<bspDrawVert_t> node_verts;
		std::vector<bspDrawIndex_t> node_indices;

#if 0
		std::vector<bspDrawIndex_t> temp_indices;
#endif

		node_faces.reserve( node_materials.size() );

		// loop 2: node > triangulated indices
		for ( uint iIndices = 0; iIndices < (uint)triangulated_indices.size(); ++iIndices )
		{
			const richIndexData_t &index = triangulated_indices[iIndices];

			bool skipVert = false;

#if 0
			// loop 3: node > indices > new verts
			for ( uint iFatVerts = 0; iFatVerts < (uint)fatVerts.size(); ++iFatVerts )
			{
				fatVert_t &fatVert = fatVerts[iFatVerts];
				// have we created this exact vert already?
				if ( index.texInfo == fatVert.rich.texInfo )
				{
					if ( index.index == fatVert.rich.index )
					{
						// use this one
						temp_indices.push_back( iFatVerts );
						skipVert = true;
					}
				}
			}
#endif

			if ( skipVert )
			{
				continue;
			}

			// create a new fat vert

			texinfo_t texInfo = g_texinfos[index.texInfo];

			bspDrawVert_t drawVert{};
			
			drawVert.xyz.SetFromLegacy( g_vertices[index.index].point );
			drawVert.st.x = DotProduct( drawVert.xyz.Base(), texInfo.vecs[0] ) + texInfo.vecs[0][3];	// must be /= width at runtime
			drawVert.st.y = DotProduct( drawVert.xyz.Base(), texInfo.vecs[1] ) + texInfo.vecs[1][3];	// must be /= height at runtime

			fatVerts.push_back( { drawVert, index } );

#if 0
			temp_indices.push_back( static_cast<uint16>( fatVerts.size() - 1 ) );
#endif
		}

		node_verts.reserve( fatVerts.size() );

		// TODO: CHECK FOR OFF BY ONE!!!!!!

		// loop 2: collapse fat verts
		for ( uint iFatVerts = 0; iFatVerts < (uint)fatVerts.size(); ++iFatVerts )
		{
			fatVert_t &fatVert = fatVerts[iFatVerts];

			node_verts.push_back( fatVert.drawVert );
		}

		// create the faces

		node_faces.reserve( node_materials.size() );

		// loop 2: node > materials used
		for ( uint iMats = 0; iMats < (uint)node_materials.size(); ++iMats )
		{
			auto &mat = node_materials[iMats];
			auto &face = node_faces.emplace_back();

			face.firstIndex = (bspDrawIndex_t)( indexOffset2 );
			face.numIndices = 0;

			// loop 3: node > faces > fat verts
			for ( uint iFatVerts = 0; iFatVerts < (uint)fatVerts.size(); ++iFatVerts )
			{
				// find all the fat verts with this material and stick them in the index buffer
				fatVert_t &fatVert = fatVerts[iFatVerts];

				const auto &texInfo = g_texinfos[fatVert.rich.texInfo];

				if ( Q_strcmp( texInfo.texture, mat.name.c_str() ) != 0 )
				{
					// nope, belongs to another chain
					continue;
				}

				node_indices.push_back( iFatVerts );
				++face.numIndices;
				++indexOffset2;
			}


			int fuck = 0;
		}

		// spit into the global bsp array

		bool first = true;

		for ( uint iFaces = 0; iFaces < (uint)node_faces.size(); ++iFaces )
		{
			if ( first )
			{
				node.newFirstFace = Max( 0, (int)g_drawFaces.size() - 1 );
				first = false;
			}
			g_drawFaces.push_back( node_faces[iFaces] );
		}

		node.newNumFaces = static_cast<uint32>( g_drawFaces.size() - node.newFirstFace );

		for ( uint iVerts = 0; iVerts < (uint)node_verts.size(); ++iVerts )
		{
			g_drawVerts.push_back( node_verts[iVerts] );
		}

		for ( uint iIndices = 0; iIndices < (uint)node_indices.size(); ++iIndices )
		{
			uint16 index = node_indices[iIndices];

			index += indexOffset; // HACK

			g_drawIndices.push_back( index );
		}

		assert( indexOffset2 == (uint32)g_drawIndices.size() );
		indexOffset += (uint32)g_drawIndices.size();
	}

	Com_Print( "holy shit\n" );
}

void Decompose_WriteOBJ()
{
	FILE *objHandle = fopen( "data.obj", "wb" );
	if ( !objHandle )
	{
		return;
	}

	for ( uint iNode = 0; iNode < (uint)g_nodes.size(); ++iNode )
	{
		const dnode_t &node = g_nodes[iNode];

		// write all the verts
		for ( uint32 iFace = 0; iFace < node.newNumFaces; ++iFace )
		{
			const bspDrawFace_t &face = g_drawFaces[node.newFirstFace + iFace];

			for ( uint32 iIndex = 0; iIndex < face.numIndices; ++iIndex )
			{
				bspDrawIndex_t index = g_drawIndices[face.firstIndex + iIndex];

				const bspDrawVert_t vert = g_drawVerts[index];

				fprintf( objHandle, "v %g %g %g\n", vert.xyz.x, vert.xyz.y, vert.xyz.z );
				fprintf( objHandle, "vt %g %g\n", vert.st.x, vert.st.y );
			}
		}

		fprintf( objHandle, "o node%u\n", iNode );

#if 1
		// write all the faces
		for ( uint32 iFace = 0; iFace < node.newNumFaces; ++iFace )
		{
			const bspDrawFace_t &face = g_drawFaces[node.newFirstFace + iFace];

			if ( face.numIndices % 3 )
			{
				Com_Print( "Found a face with bad triangulation\n" );
				continue;
			}

			for ( uint32 iIndex = 0; iIndex < face.numIndices; iIndex += 3 )
			{
				bspDrawIndex_t index1 = g_drawIndices[face.firstIndex + iIndex + 0];
				bspDrawIndex_t index2 = g_drawIndices[face.firstIndex + iIndex + 1];
				bspDrawIndex_t index3 = g_drawIndices[face.firstIndex + iIndex + 2];

				fprintf( objHandle, "f %u/%u %u/%u %u/%u\n", index1, index1, index2, index2, index3, index3 );
			}
		}
#endif
	}

	fclose( objHandle );
}

static void Decompose_FixNodes( byte *bspBuffer, lump_t *lump )
{
	dnode_t *nodes = (dnode_t *)( bspBuffer + lump->fileofs );

	memcpy( nodes, g_nodes.data(), g_nodes.size() * sizeof( dnode_t ) );
}

bool Decompose_Write( const char *bspFilename, byte *oldBspBuffer, size_t oldBspLength )
{
	// mod the header
	dheader_t *pHeader = (dheader_t *)oldBspBuffer;

	size_t drawFacesSize = g_drawFaces.size() * sizeof( bspDrawFace_t );
	size_t drawVertsSize = g_drawVerts.size() * sizeof( bspDrawVert_t );
	size_t drawIndicesSize = g_drawIndices.size() * sizeof( bspDrawIndex_t );

	pHeader->lumps[LUMP_DRAWFACES].fileofs = static_cast<int>( oldBspLength );
	pHeader->lumps[LUMP_DRAWFACES].filelen = static_cast<int>( drawFacesSize );

	Com_Printf( "DrawFaces offset: %d\n", pHeader->lumps[LUMP_DRAWFACES].fileofs );
	Com_Printf( "DrawFaces length: %d\n", pHeader->lumps[LUMP_DRAWFACES].filelen );

	pHeader->lumps[LUMP_DRAWVERTS].fileofs = static_cast<int>( oldBspLength + drawFacesSize );
	pHeader->lumps[LUMP_DRAWVERTS].filelen = static_cast<int>( drawVertsSize );

	Com_Printf( "DrawVerts offset: %d\n", pHeader->lumps[LUMP_DRAWVERTS].fileofs );
	Com_Printf( "DrawVerts length: %d\n", pHeader->lumps[LUMP_DRAWVERTS].filelen );

	pHeader->lumps[LUMP_DRAWINDICES].fileofs = static_cast<int>( oldBspLength + drawFacesSize + drawVertsSize );
	pHeader->lumps[LUMP_DRAWINDICES].filelen = static_cast<int>( drawIndicesSize );

	Com_Printf( "DrawIndices offset: %d\n", pHeader->lumps[LUMP_DRAWINDICES].fileofs );
	Com_Printf( "DrawIndices length: %d\n", pHeader->lumps[LUMP_DRAWINDICES].filelen );

	// fixup nodes

	Decompose_FixNodes( oldBspBuffer, &pHeader->lumps[LUMP_NODES] );

	size_t newBspLength = oldBspLength + drawFacesSize + drawVertsSize + drawIndicesSize;

	// open and clear the bsp

	FILE *newBspHandle = fopen( bspFilename, "wb" );
	if ( !newBspHandle )
	{
		Com_Printf( "Couldn't open %s for writing\n", bspFilename );
		return false;
	}

	// write the initial chunk of the old BSP

	fwrite( oldBspBuffer, oldBspLength, 1, newBspHandle );

	// append the new stuff

	fwrite( g_drawFaces.data(), g_drawFaces.size(), sizeof( bspDrawFace_t ), newBspHandle );
	fwrite( g_drawVerts.data(), g_drawVerts.size(), sizeof( bspDrawVert_t ), newBspHandle );
	fwrite( g_drawIndices.data(), g_drawIndices.size(), sizeof( bspDrawIndex_t ), newBspHandle );

	fclose( newBspHandle );

	return true;
}
