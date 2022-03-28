/*
===================================================================================================

	World Surface Rendering

	END RESULT AFTER MAP LOADING:
	One large trilist sorted by material
	Each surface retains the texinfo it uses, and the index into the trilist and count
	When finding faces to render, keep a 

===================================================================================================
*/

#include "gl_local.h"

#include "../shared/imgtools.h"
#include <algorithm> // std::sort
#include <vector>

#define STBI_NO_STDIO
#define STBI_ONLY_HDR
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "../../thirdparty/stb/stb_image.h"

// Comment out to disable meshoptimizer optimisations
#define USE_MESHOPT

// Experimental atlasing stuff
#define USE_XATLAS

#ifdef USE_MESHOPT
#define MESHOPTIMIZER_NO_WRAPPERS
#include "../../thirdparty/meshoptimizer/src/meshoptimizer.h"
#endif

#ifdef USE_XATLAS
#include "../../thirdparty/xatlas/xatlas.h"
#endif

#define MAX_SHADERLIGHTS	4

#define LIGHTMAP_BYTES		4

// Width and height of the lightmap atlas
#define	BLOCK_WIDTH		1024
#define	BLOCK_HEIGHT	1024

// How many lightmap atlases we can have
#define	MAX_LIGHTMAPS	8

#define GL_LIGHTMAP_INTERNAL_FORMAT		GL_RGBA8 // GL_SRGB8_ALPHA8
#define GL_LIGHTMAP_FORMAT				GL_RGBA

// Globals

worldRenderData_t g_worldData;

struct worldLists_t
{
	std::vector<worldIndex_t>	finalIndices;		// Indices into the render data used to draw the PVS
	std::vector<worldMesh_t>	opaqueMeshes;		// Each worldMesh_t is a draw call

	void ClearAllLists()
	{
		finalIndices.clear();
		opaqueMeshes.clear();
	}
};

static vec3_t modelorg;		// relative to viewpoint

struct gllightmapstate_t
{
	GLuint			lightmapTextures[MAX_LIGHTMAPS];
	GLuint			currentLightmapTexture;

	msurface_t *	lightmap_surfaces[MAX_LIGHTMAPS];

	int				allocated[BLOCK_WIDTH];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte			lightmap_buffer[4*BLOCK_WIDTH*BLOCK_HEIGHT];
};

static gllightmapstate_t gl_lms;

// gl_light
void R_SetCacheState(msurface_t *surf);
void R_BuildLightMap(msurface_t *surf, byte *dest, int stride);

static StaticCvar r_fastProfile( "r_fastProfile", "0", 0 );

/*
===================================================================================================

	BRUSH MODELS

===================================================================================================
*/

//
// Returns the proper texture for a given time and base texture
//
static material_t *R_TextureAnimation( const mtexinfo_t *tex )
{
	if ( !tex->next )
	{
		return tex->material;
	}

	for ( int c = currententity->frame % tex->numframes; c; --c )
	{
		tex = tex->next;
	}

	return tex->material;
}

static void R_DrawWorldMesh( const worldMesh_t &mesh )
{
	ZoneScoped

	const material_t *mat = R_TextureAnimation( mesh.texinfo );

	GL_ActiveTexture( GL_TEXTURE0 );
	mat->Bind();
	GL_ActiveTexture( GL_TEXTURE1 );
	GL_BindTexture( g_worldData.lightmapTexnum ? g_worldData.lightmapTexnum : gl_lms.lightmapTextures[1] );
	GL_ActiveTexture( GL_TEXTURE2 );
	mat->BindSpec();
	GL_ActiveTexture( GL_TEXTURE3 );
	mat->BindEmit();

	tr.pc.worldPolys += mesh.numIndices / 3;
	++tr.pc.worldDrawCalls;
	
	glDrawElements( GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, (void *)( (uintptr_t)( mesh.firstIndex ) * sizeof( worldIndex_t ) ) );
}

//
// Draw water surfaces and windows.
// The BSP tree is waled front to back, so unwinding the chain
// of alpha_surfaces will draw back to front, giving proper ordering.
//
void R_DrawAlphaSurfaces()
{

}

//
// Returns true if the box is completely outside the frustum
//
static bool R_CullBox( const vec3_t mins, const vec3_t maxs )
{
	if ( r_nocull->GetBool() ) {
		return false;
	}

	for ( int i = 0; i < 4; ++i )
	{
		if ( BoxOnPlaneSide( mins, maxs, &frustum[i] ) == 2 ) {
			return true;
		}
	}

	return false;
}

//
// Draws an entity that uses a brush model
//
void R_DrawBrushModel( entity_t *e )
{
#if 1
#if 1
	vec3_t		mins, maxs;
	bool		rotated;

	if ( currentmodel->numMeshes == 0 )
	{
		return;
	}

	currententity = e;

	if ( e->angles[0] || e->angles[1] || e->angles[2] )
	{
		rotated = true;
		for ( uint i = 0; i < 3; ++i )
		{
			mins[i] = e->origin[i] - currentmodel->radius;
			maxs[i] = e->origin[i] + currentmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd( e->origin, currentmodel->mins, mins );
		VectorAdd( e->origin, currentmodel->maxs, maxs );
	}

	//if ( R_CullBox( mins, maxs ) )
	//{
	//	return;
	//}

	memset( gl_lms.lightmap_surfaces, 0, sizeof( gl_lms.lightmap_surfaces ) );

	VectorSubtract( tr.refdef.vieworg, e->origin, modelorg );
	if ( rotated )
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy( modelorg, temp );
		AngleVectors( e->angles, forward, right, up );
		modelorg[0] = DotProduct( temp, forward );
		modelorg[1] = -DotProduct( temp, right );
		modelorg[2] = DotProduct( temp, up );
	}

	using namespace DirectX;

#if 1
	XMMATRIX modelMatrix = XMMatrixMultiply(
		XMMatrixRotationX( DEG2RAD( e->angles[ROLL] ) ) * XMMatrixRotationY( DEG2RAD( e->angles[PITCH] ) ) * XMMatrixRotationZ( DEG2RAD( e->angles[YAW] ) ),
		XMMatrixTranslation( e->origin[0], e->origin[1], e->origin[2] )
	);
#else
	XMMATRIX modelMatrix = XMMatrixIdentity();
#endif

	XMFLOAT4X4A modelMatrixStore;
	XMStoreFloat4x4A( &modelMatrixStore, modelMatrix );

	glBindVertexArray( g_worldData.vao );
	glBindBuffer( GL_ARRAY_BUFFER, g_worldData.vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, g_worldData.eboSubmodels );

	GL_UseProgram( glProgs.worldProg );

	glUniformMatrix4fv( 4, 1, GL_FALSE, (const GLfloat *)&modelMatrixStore );

	worldMesh_t *firstMesh = g_worldData.meshes.data() + currentmodel->firstMesh;
	worldMesh_t *lastMesh = firstMesh + currentmodel->numMeshes;

	for ( worldMesh_t *mesh = firstMesh; mesh < lastMesh; ++mesh )
	{
		R_DrawWorldMesh( *mesh );
	}
#else
	vec3_t		mins, maxs;
	int			i;
	qboolean	rotated;

	if (currentmodel->nummodelsurfaces == 0)
		return;

	currententity = e;
	glState.currenttextures[0] = glState.currenttextures[1] = -1;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = e->origin[i] - currentmodel->radius;
			maxs[i] = e->origin[i] + currentmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd (e->origin, currentmodel->mins, mins);
		VectorAdd (e->origin, currentmodel->maxs, maxs);
	}

	if (R_CullBox (mins, maxs))
		return;

	glColor3f (1,1,1);
	memset (gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

	VectorSubtract (tr.refdef.vieworg, e->origin, modelorg);
	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (e->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

	glPushMatrix ();
e->angles[0] = -e->angles[0];	// stupid quake bug
e->angles[2] = -e->angles[2];	// stupid quake bug
	R_RotateForEntity (e);
e->angles[0] = -e->angles[0];	// stupid quake bug
e->angles[2] = -e->angles[2];	// stupid quake bug

	GL_EnableMultitexture( true );
	GL_SelectTexture( GL_TEXTURE0);
	GL_TexEnv( GL_REPLACE );
	GL_SelectTexture( GL_TEXTURE1);
	GL_TexEnv( GL_MODULATE );

	R_DrawInlineBModel ();
	GL_EnableMultitexture( false );

	glPopMatrix ();
#endif
#endif
}

/*
===================================================================================================

	WORLD MODEL

===================================================================================================
*/

// When recursing the BSP tree we give each material its own array of indices, then
// when we finish, we append them all into one large array which we send to the GPU
struct worldMaterialSet_t
{
	mtexinfo_t *texinfo;
	std::vector<worldIndex_t> indices;
};

struct worldNodeWork_t
{
	std::vector<worldMaterialSet_t> materialSets;
};

// Mark the leaves and nodes that are in the PVS for the current cluster
static void R_MarkLeaves()
{
	ZoneScoped

	byte *		vis;
	byte		fatvis[MAX_MAP_LEAFS / 8];
	mnode_t *	node;
	int			i, c;
	mleaf_t *	leaf;
	int			cluster;

	if ( r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->GetBool() && r_viewcluster != -1 )
	{
		return;
	}

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if ( r_lockpvs->GetBool() )
	{
		return;
	}

	++tr.visCount;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if ( r_novis->GetBool() || r_viewcluster == -1 || !r_worldmodel->vis )
	{
		// mark everything
		for ( i = 0; i < r_worldmodel->numleafs; i++ )
		{
			r_worldmodel->leafs[i].visframe = tr.visCount;
		}
		for ( i = 0; i < r_worldmodel->numnodes; i++ )
		{
			r_worldmodel->nodes[i].visframe = tr.visCount;
		}
		return;
	}

	vis = Mod_ClusterPVS( r_viewcluster, r_worldmodel );

	// may have to combine two clusters because of solid water boundaries
	if ( r_viewcluster2 != r_viewcluster )
	{
		memcpy( fatvis, vis, ( r_worldmodel->numleafs + 7 ) / 8 );
		vis = Mod_ClusterPVS( r_viewcluster2, r_worldmodel );
		c = ( r_worldmodel->numleafs + 31 ) / 32;
		for ( i = 0; i < c; i++ )
		{
			( (int *)fatvis )[i] |= ( (int *)vis )[i];
		}
		vis = fatvis;
	}

	for ( i = 0, leaf = r_worldmodel->leafs; i < r_worldmodel->numleafs; ++i, ++leaf )
	{
		cluster = leaf->cluster;

		if ( cluster == -1 )
		{
			continue;
		}

		// Check general pvs
		if ( !( vis[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) )
		{
			// not visible
			continue;
		}

		node = (mnode_t *)leaf;
		do
		{
			if ( node->visframe == tr.visCount )
			{
				break;
			}
			node->visframe = tr.visCount;
			node = node->parent;
		} while ( node );
	}

#if 0
	for ( i = 0; i < r_worldmodel->vis->numclusters; i++ )
	{
		if ( vis[i >> 3] & ( 1 << ( i & 7 ) ) )
		{
			node = (mnode_t *)&r_worldmodel->leafs[i];	// FIXME: cluster
			do
			{
				if ( node->visframe == tr.visCount )
					break;
				node->visframe = tr.visCount;
				node = node->parent;
			} while ( node );
		}
	}
#endif
}

static void R_AddSurfaceToMaterialSet( const msurface_t *surf, worldMaterialSet_t &materialSet )
{
	if ( surf->texinfoFlags & SURF_SKY )
	{
		R_AddSkySurface( surf );
		return;
	}

	const auto begin = g_worldData.indices.begin() + surf->firstIndex;
	const auto end = begin + surf->numIndices;

	materialSet.indices.insert( materialSet.indices.end(), begin, end );

	//assert( !( materialSet.indices.size() % 3 ) );
}

static void R_AddSurface( const msurface_t *surf, worldNodeWork_t &work )
{
#if 1
	bool added = false;
	for ( worldMaterialSet_t &materialSet : work.materialSets )
	{
		if ( materialSet.texinfo->material == surf->texinfo->material )
		{
			// Insert into this material set
			R_AddSurfaceToMaterialSet( surf, materialSet );
			added = true;
		}
	}
	if ( !added )
	{
		// Create a new material set
		worldMaterialSet_t &materialSet = work.materialSets.emplace_back();
		materialSet.texinfo = surf->texinfo;
		R_AddSurfaceToMaterialSet( surf, materialSet );
	}
#else
	glBegin( GL_TRIANGLES );
	for ( uint32 i = surf->firstIndex; i < surf->firstIndex + surf->numIndices; ++i )
	{
		const worldVertex_t &vertex = g_worldData.vertices[g_worldData.indices[i]];
		glVertex3fv( (float *)&vertex.pos );
	}
	glEnd();
#endif
}

//
// Recurses down the BSP tree inserting surfaces that need to be drawn
// into the world lists from back to front (TODO: make it front to back)
//
static void R_RecursiveWorldNode( worldNodeWork_t &work, const mnode_t *node )
{
	ZoneScoped

	while ( true )
	{
		// No polygons in solid nodes
		if ( node->contents == CONTENTS_SOLID ) {
			return;
		}

		// If the node wasn't marked by R_MarkLeaves, exit
		if ( node->visframe != tr.visCount ) {
			return;
		}

		// Frustum cull
		if ( R_CullBox( node->mins, node->maxs ) ) {
			return;
		}

		// if a leaf node, mark surfaces
		if ( node->contents != -1 )
		{
			const mleaf_t *leaf = (mleaf_t *)node;

			// Check for door connected areas
			if ( !( tr.refdef.areabits[leaf->area >> 3] & ( 1 << ( leaf->area & 7 ) ) ) )
			{
				// not visible
				return;
			}

			msurface_t **firstMarkSurface = r_worldmodel->marksurfaces + leaf->firstmarksurface;
			msurface_t **lastMarkSurface = firstMarkSurface + leaf->nummarksurfaces;

			for ( msurface_t **mark = firstMarkSurface; mark < lastMarkSurface; ++mark )
			{
				// Mark this surface to be drawn at the node
				( *mark )->frameCount = tr.frameCount;
			}

			return;
		}

		// node is just a decision point, so go down the appropriate sides

		// find which side of the node we are on
		const cplane_t *plane = node->plane;
		float dot;

		switch ( plane->type )
		{
		case PLANE_X:
			dot = modelorg[0] - plane->dist;
			break;
		case PLANE_Y:
			dot = modelorg[1] - plane->dist;
			break;
		case PLANE_Z:
			dot = modelorg[2] - plane->dist;
			break;
		default:
			dot = DotProduct( modelorg, plane->normal ) - plane->dist;
			break;
		}

		int side, sidebit;

		if ( dot >= 0 )
		{
			side = 0;
			sidebit = 0;
		}
		else
		{
			side = 1;
			sidebit = MSURF_PLANEBACK;
		}

		// Recurse down the children, front side first
		R_RecursiveWorldNode( work, node->children[side] );

		// draw stuff

		const msurface_t *firstSurface = r_worldmodel->surfaces + node->firstsurface;
		const msurface_t *lastSurface = firstSurface + node->numsurfaces;

		for ( const msurface_t *surf = firstSurface; surf < lastSurface; ++surf )
		{
			if ( surf->frameCount != tr.frameCount )
			{
				// Not visited in the leafs
				continue;
			}

			if ( ( surf->flags & MSURF_PLANEBACK ) != sidebit )
			{
				// Surface is facing away from us
				continue;
			}

			R_AddSurface( surf, work );
		}

		// Recurse down the back side
		// We can prevent a recursion by just using this while loop
		//R_RecursiveWorldNode( work, node->children[!side] );
		node = node->children[!side];
	}
}

//
// Squashes our index lists into the final index buffer, builds the world lists
//
static void R_SquashAndUploadIndices( worldLists_t &worldLists, const worldNodeWork_t &work )
{
	ZoneScoped

	if ( work.materialSets.empty() )
	{
		return;
	}

	for ( const worldMaterialSet_t &materialSet : work.materialSets )
	{
		// Create a new mesh
		worldMesh_t &opaqueMesh = worldLists.opaqueMeshes.emplace_back();
		opaqueMesh.texinfo = materialSet.texinfo;

		uint32 firstIndex = static_cast<uint32>( worldLists.finalIndices.size() );

		// Insert our individual index buffer at the end of the final index buffer
		worldLists.finalIndices.insert( worldLists.finalIndices.end(), materialSet.indices.begin(), materialSet.indices.end() );

		uint32 end = static_cast<uint32>( worldLists.finalIndices.size() );

		assert( end - firstIndex == materialSet.indices.size() );

		opaqueMesh.firstIndex = firstIndex;
		opaqueMesh.numIndices = materialSet.indices.size();
	}

	const void *indexData = reinterpret_cast<const void *>( worldLists.finalIndices.data() );
	const GLsizeiptr indexSize = static_cast<GLsizeiptr>( worldLists.finalIndices.size() ) * sizeof( worldIndex_t );

	glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexSize, indexData, GL_DYNAMIC_DRAW );
}

//
// Draws all opaque surfaces in the world list
//
static void R_DrawStaticOpaqueWorld( const worldLists_t &worldLists )
{
	ZoneScoped

	for ( const worldMesh_t &mesh : worldLists.opaqueMeshes )
	{
		R_DrawWorldMesh( mesh );
	}
}

//
// Main entry point for drawing static, opaque world geometry
//
void R_DrawWorld()
{
	ZoneScoped

	if ( !r_drawworld->GetBool() ) {
		return;
	}

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return;
	}

	// Code that doesn't set areabits will never get here
	assert( tr.refdef.areabits );

	currentmodel = r_worldmodel;

	VectorCopy( tr.refdef.vieworg, modelorg );

	// The world entity
	entity_t ent;

	// Auto cycle the world frame for texture animation
	memset( &ent, 0, sizeof( ent ) );
	ent.frame = (int)( tr.refdef.time * 2 );
	currententity = &ent;

	memset( gl_lms.lightmap_surfaces, 0, sizeof( gl_lms.lightmap_surfaces ) );

	R_ClearSkyBox();

	// SHADERWORLD

	glBindVertexArray( g_worldData.vao );
	glBindBuffer( GL_ARRAY_BUFFER, g_worldData.vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, g_worldData.ebo );

	// Build the world lists

	worldLists_t worldLists;

	{
		worldNodeWork_t work;

		// Determine which leaves are in the PVS / areamask
		R_MarkLeaves();

#if 0
		int64 start, end;

		if ( r_fastProfile.GetBool() )
		{
			start = Time_Microseconds();
		}
#endif

		// This figures out what we need to render and builds the world lists
		R_RecursiveWorldNode( work, r_worldmodel->nodes );

#if 0
		if ( r_fastProfile.GetBool() )
		{
			end = Time_Microseconds();

			Cvar_SetBool( &r_fastProfile, false );

			int64 timeTaken = end - start;

			Com_Printf( "Spent %llu microseconds inside R_RecursiveWorldNode\n", timeTaken );
		}
#endif

		R_SquashAndUploadIndices( worldLists, work );
	}

	// Create the model matrix
	DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixIdentity();
	DirectX::XMFLOAT4X4A modelMatrixStore;
	DirectX::XMStoreFloat4x4A( &modelMatrixStore, modelMatrix );

	// Set up shader state
	GL_UseProgram( glProgs.worldProg );

	glUniformMatrix4fv( 4, 1, GL_FALSE, (const GLfloat *)&modelMatrixStore );
	glUniformMatrix4fv( 5, 1, GL_FALSE, (const GLfloat *)&tr.viewMatrix );
	glUniformMatrix4fv( 6, 1, GL_FALSE, (const GLfloat *)&tr.projMatrix );

	glUniform3fv( 7, 1, tr.refdef.vieworg );

	constexpr int startIndex = 8;
	constexpr int elementsInRenderLight = 3;

#if 1
	for ( int iter1 = 0, iter2 = 0; iter1 < MAX_SHADERLIGHTS; ++iter1, iter2 += elementsInRenderLight )
	{
		glUniform3fv( startIndex + iter2 + 0, 1, tr.refdef.dlights[iter1].origin );
		glUniform3fv( startIndex + iter2 + 1, 1, tr.refdef.dlights[iter1].color );
		glUniform1f( startIndex + iter2 + 2, tr.refdef.dlights[iter1].intensity );
	}
#endif

	constexpr int indexAfterLights = startIndex + elementsInRenderLight * MAX_SHADERLIGHTS;

	glUniform1i( indexAfterLights + 0, 0 ); // diffuse
	glUniform1i( indexAfterLights + 1, 1 ); // lightmap
	glUniform1i( indexAfterLights + 2, 2 ); // spec
	glUniform1i( indexAfterLights + 3, 3 ); // spec

	// Render stuff!
	R_DrawStaticOpaqueWorld( worldLists );

	GL_UseProgram( 0 );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );

	R_DrawSkyBox();

	//R_DrawTriangleOutlines();
}

/*
===================================================================================================

	Lightmap Allocation

	This code is called during the loading of faces in gl_model.cpp

===================================================================================================
*/

static void LM_InitBlock()
{
	memset( gl_lms.allocated, 0, sizeof( gl_lms.allocated ) );
}

static void LM_UploadBlock( bool dynamic )
{
	int texture = dynamic ? 0 : gl_lms.currentLightmapTexture;

	if ( gl_lms.lightmapTextures[texture] == 0 )
	{
		glGenTextures( 1, &gl_lms.lightmapTextures[texture] );
	}

	GL_ActiveTexture( GL_TEXTURE1 );
	GL_BindTexture( gl_lms.lightmapTextures[texture] );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	if ( dynamic )
	{
		int i = 0;
		int height = 0;

		for ( ; i < BLOCK_WIDTH; ++i )
		{
			if ( gl_lms.allocated[i] > height )
			{
				height = gl_lms.allocated[i];
			}
		}

		glTexSubImage2D( GL_TEXTURE_2D,
			0,
			0, 0,
			BLOCK_WIDTH, height,
			GL_LIGHTMAP_FORMAT,
			GL_UNSIGNED_BYTE,
			gl_lms.lightmap_buffer );
	}
	else
	{
		glTexImage2D( GL_TEXTURE_2D,
			0,
			GL_LIGHTMAP_INTERNAL_FORMAT,
			BLOCK_WIDTH, BLOCK_HEIGHT,
			0,
			GL_LIGHTMAP_FORMAT,
			GL_UNSIGNED_BYTE,
			gl_lms.lightmap_buffer );

		++gl_lms.currentLightmapTexture;

		if ( gl_lms.currentLightmapTexture == MAX_LIGHTMAPS )
		{
			Com_Error( "LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n" );
		}
	}
}

//
// returns a texture number and the position inside it
//
static bool LM_AllocBlock( int w, int h, int *x, int *y )
{
	int i, j;
	int best, best2;

	best = BLOCK_HEIGHT;

	for ( i = 0; i < BLOCK_WIDTH - w; i++ )
	{
		best2 = 0;

		for ( j = 0; j < w; j++ )
		{
			if ( gl_lms.allocated[i + j] >= best )
			{
				break;
			}
			if ( gl_lms.allocated[i + j] > best2 )
			{
				best2 = gl_lms.allocated[i + j];
			}
		}
		if ( j == w )
		{
			// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if ( best + h > BLOCK_HEIGHT )
	{
		return false;
	}

	for ( i = 0; i < w; i++ )
	{
		gl_lms.allocated[*x + i] = best + h;
	}

	return true;
}

//
// This should only ever be called between
// GL_BeginBuildingLightmaps and
// GL_EndBuildingLightmaps
//
void GL_CreateSurfaceLightmap( msurface_t *surf )
{
	if ( surf->flags & SURFMASK_UNLIT )
	{
		return;
	}

	int smax = ( surf->extents[0] >> 4 ) + 1;
	int tmax = ( surf->extents[1] >> 4 ) + 1;

	if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
	{
		LM_UploadBlock( false );
		LM_InitBlock();
		if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
		{
			Com_FatalErrorf( "Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax );
		}
	}

	surf->lightmaptexturenum = gl_lms.currentLightmapTexture;

	byte *base = gl_lms.lightmap_buffer;
	base += ( surf->light_t * BLOCK_WIDTH + surf->light_s ) * LIGHTMAP_BYTES;

	R_SetCacheState( surf );
	R_BuildLightMap( surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES );
}

void GL_BeginBuildingLightmaps( model_t *model )
{
	static lightstyle_t	lightstyles[MAX_LIGHTSTYLES];

	memset( gl_lms.allocated, 0, sizeof( gl_lms.allocated ) );

	tr.frameCount = 1; // no dlightcache

	/*
	** setup the base lightstyles so the lightmaps won't have to be regenerated
	** the first time they're seen
	*/
	for ( int i = 0; i < MAX_LIGHTSTYLES; ++i )
	{
		lightstyles[i].rgb[0] = 1.0f;
		lightstyles[i].rgb[1] = 1.0f;
		lightstyles[i].rgb[2] = 1.0f;
		lightstyles[i].white = 3.0f;
	}
	tr.refdef.lightstyles = lightstyles;

	gl_lms.currentLightmapTexture = 1;

	/*
	** initialize the dynamic lightmap texture
	*/
	glGenTextures( 1, &gl_lms.lightmapTextures[0] );

	GL_ActiveTexture( GL_TEXTURE1 );
	GL_BindTexture( gl_lms.lightmapTextures[0] );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D,
		0,
		GL_LIGHTMAP_INTERNAL_FORMAT,
		BLOCK_WIDTH, BLOCK_HEIGHT,
		0,
		GL_LIGHTMAP_FORMAT,
		GL_UNSIGNED_BYTE,
		nullptr );
}

void GL_EndBuildingLightmaps()
{
	LM_UploadBlock( false );
}

// Kept here for reference
#if 0
static void R_BuildVertexNormals( worldVertex_t &v1, worldVertex_t &v2, worldVertex_t &v3 )
{
	vec3_t a, b, normal;
	VectorSubtract( v2.pos, v1.pos, a );
	VectorSubtract( v3.pos, v1.pos, b );

	CrossProduct( b, a, normal );
	VectorNormalize( normal );

	VectorCopy( normal, v1.normal );
	VectorCopy( normal, v2.normal );
	VectorCopy( normal, v3.normal );
}
#endif

// Move to mathlib or types
static bool IsZero( float f, float epsilon )
{
	return fabs( f ) <= epsilon;
}

static float R_TriangleArea( const worldVertex_t &v0, const worldVertex_t &v1, const worldVertex_t &v2 )
{
	vec3_t edge0, edge1, cross;
	VectorSubtract( v1.pos, v0.pos, edge0 );
	VectorSubtract( v2.pos, v0.pos, edge1 );
	CrossProduct( edge0, edge1, cross );
	return VectorLength( cross ) * 0.5f;
}

//
// Given an msurface_t with basic brush geometry specified
// will create optimised render geometry and append it to g_worldData
// It then gives the index of the first renderindex to the msurface_t
//
static void R_BuildPolygonFromSurface( msurface_t *fa, int skipIndices )
{
	ZoneScoped

	const int *pSurfEdges = currentmodel->surfedges;
	const medge_t *pEdges = currentmodel->edges;
	const mvertex_t *pVertices = currentmodel->vertexes;
	const int numEdges = fa->numedges; // Equal to the number of vertices

	assert( numEdges >= 3 );

	// Helper lambda to get a vertex for a surface
	auto R_GetVertexForSurface =
		[pSurfEdges, pEdges, pVertices]( msurface_t *fa, int index )
	{
		const int surfEdge = pSurfEdges[fa->firstedge + index];
		const int startIndex = surfEdge > 0 ? 0 : 1;

		const medge_t *edge = pEdges + abs( surfEdge );
		const float *vertex = pVertices[edge->v[startIndex]].position;

		return vertex;
	};

	// Index of the first vertex
	const uint32 firstVertex = static_cast<uint32>( g_worldData.vertices.size() );

	// Reconstruct the polygon

	// First, build the vertices for this face

	for ( int i = 0; i < numEdges; ++i )
	{
		const float *pVertex = R_GetVertexForSurface( fa, i );

		worldVertex_t &outVertex = g_worldData.vertices.emplace_back();

		// ST/UV coordinates

		float s, t;

		s = DotProduct( pVertex, fa->texinfo->vecs[0] ) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->material->image->width;

		t = DotProduct( pVertex, fa->texinfo->vecs[1] ) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->material->image->height;

		// Copy in the vertex positions and ST coordinates

		VectorCopy( pVertex, outVertex.pos );

		outVertex.st1[0] = s;
		outVertex.st1[1] = t;

		// Lightmap ST/UV coordinates

		s = DotProduct( pVertex, fa->texinfo->vecs[0] ) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16; // fa->texinfo->texture->width;

		t = DotProduct( pVertex, fa->texinfo->vecs[1] ) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16; // fa->texinfo->texture->height;

		outVertex.st2[0] = s;
		outVertex.st2[1] = t;

		VectorCopy( fa->plane->normal, outVertex.normal );
		if ( fa->flags & MSURF_PLANEBACK )
		{
			// Flip the normals
			VectorNegate( outVertex.normal, outVertex.normal );
		}
	}

	// TODO: WARP WARP WARP!!!
#if 0
	if ( out->texinfo->flags & SURF_WARP )
	{
		for ( i = 0; i < 2; ++i )
		{
			out->extents[i] = 16384;
			out->texturemins[i] = -8192;
		}
		GL_SubdivideSurface( out );	// cut up polygon for warps
	}
#endif

	// Not a valid index
	//const uint32 endVertex = static_cast<uint32>( g_worldData.vertices.size() );
	//const uint32 numVertices = endVertex - firstVertex;

	// Index of the first index!
	const uint32 firstIndex = static_cast<uint32>( g_worldData.indices.size() );

	// Triangulate it, fan style

	const int numTriangles = numEdges - 2;

	// For every triangle
	for ( int i = 0; i < numTriangles; ++i )
	{
		if ( IsZero( R_TriangleArea(
			g_worldData.vertices[firstVertex],
			g_worldData.vertices[firstVertex + i + 1],
			g_worldData.vertices[firstVertex + i + 2] ), FLT_EPSILON ) )
		{
			continue;
		}

		g_worldData.indices.push_back( firstVertex );
		g_worldData.indices.push_back( firstVertex + i + 1 );
		g_worldData.indices.push_back( firstVertex + i + 2 );
	}

	// Not a valid index
	const uint32 endIndex = static_cast<uint32>( g_worldData.indices.size() );
	const uint32 numIndices = endIndex - firstIndex;

	fa->firstIndex = firstIndex;
	fa->numIndices = numIndices;

	assert( !( fa->numIndices % 3 ) );

	//
	// Figure out where we're creating the mesh
	//
	if ( skipIndices != 0 )
	{
		if ( g_worldData.lastMaterial != fa->texinfo->material )
		{
			g_worldData.lastMaterial = fa->texinfo->material;

			// Create a new mesh entry
			worldMesh_t &mesh = g_worldData.meshes.emplace_back();

			mesh.texinfo = fa->texinfo;
			mesh.firstIndex = firstIndex - skipIndices;
			mesh.numIndices = numIndices;
		}
		else
		{
			// Append to our existing one
			worldMesh_t &mesh = g_worldData.meshes.back();
			mesh.numIndices += numIndices;
		}
	}
}

static int XAtlas_PrintCallback( const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Print( msg );

	return 0;
}

static void R_AtlasWorldLists(
	const char *modelName,
	worldVertex_t *vertices, const worldIndex_t *indices,
	uint32 numVertices, uint32 numIndices )
{
#ifdef USE_XATLAS

	xatlas::SetPrint( XAtlas_PrintCallback, true );
	xatlas::Atlas *pAtlas = xatlas::Create();

	constexpr size_t offsetNormals = offsetof( worldVertex_t, normal );
	constexpr size_t offsetUVs = offsetof( worldVertex_t, st1 );

	const xatlas::MeshDecl meshDecl
	{
		.vertexPositionData = vertices,
		.vertexNormalData = (byte *)vertices + offsetNormals,
		.vertexUvData = (byte *)vertices + offsetUVs,
		.indexData = indices,

		.vertexCount = numVertices,
		.vertexPositionStride = sizeof( worldVertex_t ),
		.vertexNormalStride = sizeof( worldVertex_t ),
		.vertexUvStride = sizeof( worldVertex_t ),
		.indexCount = numIndices,
		.indexFormat = xatlas::IndexFormat::UInt32,

		.epsilon = 0.03125f
	};

	xatlas::AddMeshError meshError = xatlas::AddMesh( pAtlas, meshDecl, 1 );
	if ( meshError != xatlas::AddMeshError::Success ) {
		xatlas::Destroy( pAtlas );
		Com_Printf( "XAtlas failed: %s\n", xatlas::StringForEnum( meshError ) );
		return;
	}

	xatlas::ChartOptions chartOptions
	{
		.maxIterations = 4
	};

	xatlas::PackOptions packOptions
	{
		.bruteForce = true
	};

	xatlas::Generate( pAtlas, chartOptions, packOptions );

	Com_Printf( "%d charts\n", pAtlas->chartCount );
	Com_Printf( "%d atlases\n", pAtlas->atlasCount );

	for ( uint32 i = 0; i < pAtlas->atlasCount; ++i )
	{
		Com_Printf( "%u: %0.2f%% utilization\n", i, pAtlas->utilization[i] * 100.0f );
	}
	Com_Printf( "%ux%u resolution\n", pAtlas->width, pAtlas->height );

	for ( uint32 i = 0; i < pAtlas->meshCount; ++i )
	{
		const xatlas::Mesh &mesh = pAtlas->meshes[i];
		for ( uint32 v = 0; v < mesh.vertexCount; ++v )
		{
			const xatlas::Vertex &vertex = mesh.vertexArray[v];
			worldVertex_t *myVertex = vertices + vertex.xref;
			myVertex->st2[0] = vertex.uv[0] / pAtlas->width;
			myVertex->st2[1] = 1.0f - ( vertex.uv[1] / pAtlas->height );
		}
	}

#if 1

	char fileBase[MAX_QPATH];
	COM_FileBase( modelName, fileBase );

	char filename[MAX_QPATH];
	Q_sprintf_s( filename, "world/%s.obj", fileBase );

	fsHandle_t file = FileSystem::OpenFileWrite( filename, FS_GAMEDIR );
	if ( !file )
	{
		xatlas::Destroy( pAtlas );
		return;
	}

	uint32 firstVertex = 0;
	for ( uint32 i = 0; i < pAtlas->meshCount; ++i )
	{
		const xatlas::Mesh &mesh = pAtlas->meshes[i];
		for ( uint32 v = 0; v < mesh.vertexCount; ++v )
		{
			const xatlas::Vertex &vertex = mesh.vertexArray[v];
			const worldVertex_t *myVertex = vertices + vertex.xref;
			FileSystem::PrintFileFmt( file, "v %g %g %g\n", myVertex->pos[0], myVertex->pos[1], myVertex->pos[2] );
			//fprintf( objHandle, "vt %g %g\n", vertex.uv[0] / pAtlas->width, 1.0f - ( vertex.uv[1] / pAtlas->height ) );
			FileSystem::PrintFileFmt( file, "vt %g %g\n", vertex.uv[0] / pAtlas->width, vertex.uv[1] / pAtlas->height );
		}

		FileSystem::PrintFileFmt( file, "o %s\n", modelName );
		FileSystem::PrintFileFmt( file, "s off\n" );

		for ( uint32_t f = 0; f < mesh.indexCount; f += 3 )
		{
			const uint32_t index1 = firstVertex + mesh.indexArray[f + 0] + 1; // 1-indexed
			const uint32_t index2 = firstVertex + mesh.indexArray[f + 1] + 1; // 1-indexed
			const uint32_t index3 = firstVertex + mesh.indexArray[f + 2] + 1; // 1-indexed
			FileSystem::PrintFileFmt( file, "f %d/%d %d/%d %d/%d\n", index1, index1, index2, index2, index3, index3 );
		}

		firstVertex += mesh.vertexCount;
	}

	FileSystem::CloseFile( file );

	xatlas::Destroy( pAtlas );

#endif

#endif
}

static void R_LoadExternalLightmap( const char *bspName )
{
	char strippedName[MAX_QPATH];
	COM_FileBase( bspName, strippedName );

	char imageName[MAX_QPATH];
	Q_sprintf_s( imageName, "world/%s.hdr", strippedName );

	byte *pBuffer;
	fsSize_t nBufLen = FileSystem::LoadFile( imageName, (void **)&pBuffer );
	if ( !pBuffer )
	{
		return;
	}

	assert( nBufLen > 32 ); // Sanity check

	int width, height;
	float *hdrData = stbi_loadf_from_memory( pBuffer, nBufLen, &width, &height, nullptr, 0 );

	FileSystem::FreeFile( pBuffer );

	if ( !hdrData )
	{
		return;
	}

	glGenTextures( 1, &g_worldData.lightmapTexnum );
	GL_ActiveTexture( GL_TEXTURE1 );
	GL_BindTexture( g_worldData.lightmapTexnum );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexImage2D( GL_TEXTURE_2D,
		0,
		GL_RGB32F,
		width, height,
		0,
		GL_RGB,
		GL_FLOAT,
		hdrData );

	Mem_Free( hdrData );
}

static void R_UploadBuffers( model_t *worldModel )
{
	worldVertex_t *vertexData = g_worldData.vertices.data();
	size_t vertexSize = g_worldData.vertices.size() * sizeof( worldVertex_t );

	bool hasSubmodels = worldModel->numsubmodels > 1;
	uint32 ignoreIndices = worldModel->submodels->numMeshes;

	worldIndex_t *indexData = g_worldData.indices.data() + ignoreIndices;
	size_t indexSize = ( g_worldData.indices.size() - ignoreIndices ) * sizeof( worldIndex_t );

	glGenVertexArrays( 1, &g_worldData.vao );
	glGenBuffers( 1, &g_worldData.vbo );
	glGenBuffers( 1, &g_worldData.ebo );
	if ( hasSubmodels )
	{
		glGenBuffers( 1, &g_worldData.eboSubmodels );
	}

	glBindVertexArray( g_worldData.vao );
	glBindBuffer( GL_ARRAY_BUFFER, g_worldData.vbo );
	if ( hasSubmodels )
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, g_worldData.eboSubmodels );
	}

	// xyz, st1, st2 normal
	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );
	glEnableVertexAttribArray( 2 );
	glEnableVertexAttribArray( 3 );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( worldVertex_t ), (void *)( 0 ) );
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( worldVertex_t ), (void *)( 3 * sizeof( GLfloat ) ) );
	glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( worldVertex_t ), (void *)( 5 * sizeof( GLfloat ) ) );
	glVertexAttribPointer( 3, 3, GL_FLOAT, GL_FALSE, sizeof( worldVertex_t ), (void *)( 7 * sizeof( GLfloat ) ) );

	glBufferData( GL_ARRAY_BUFFER, vertexSize, vertexData, GL_STATIC_DRAW );
	if ( hasSubmodels )
	{
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexSize, indexData, GL_STATIC_DRAW );
	}

	g_worldData.initialised = true;
}

void R_BuildWorldLists( model_t *model )
{
	ZoneScoped

	//
	// Reserve at least enough space to hold all the base map vertices
	// The reserve amount is a rough approximation of what a map will need,
	// not what it will actually use
	//
	g_worldData.vertices.reserve( model->numvertexes );
	g_worldData.indices.reserve( model->numvertexes * 3 );

#if 1

	// Handle the world specially, it shouldn't have a mesh entry, this is because we don't
	// reference it when using the PVS for culling (we do that per-surface) and we also
	// can't sort its surfaces, because marksurfaces references them directly and we need
	// that intact for the PVS again
	// This also means that we sadly can't use meshoptimizer on it because we regenerate
	// the index buffer every time the viscluster changes
	mmodel_t *worldModel = model->submodels;
	{
		msurface_t *firstFace = model->surfaces + worldModel->firstface;
		msurface_t *lastFace = firstFace + worldModel->numfaces;

		for ( msurface_t *surface = firstFace; surface < lastFace; ++surface )
		{
			R_BuildPolygonFromSurface( surface, 0 );
		}
	}

	// Where the world indices end
	const uint32 endWorldIndices = static_cast<uint32>( g_worldData.indices.size() );

	// Store the number of indices in the mesh stuff, for r_dumpworld
	worldModel->firstMesh = 0;
	worldModel->numMeshes = endWorldIndices;

	assert( endWorldIndices != 0 );

#endif

	mmodel_t *firstModel = model->submodels + 1; // Skip world
	mmodel_t *endModel = model->submodels + model->numsubmodels; // Not a valid submodel

	for ( mmodel_t *subModel = firstModel; subModel < endModel; ++subModel )
	{
		const uint32 firstWorldMesh = static_cast<uint32>( g_worldData.meshes.size() );
		subModel->firstMesh = firstWorldMesh;

		msurface_t *firstFace = model->surfaces + subModel->firstface;
		msurface_t *lastFace = firstFace + subModel->numfaces;

		// Sort the surfaces by model, by material
		std::sort( firstFace, lastFace,
			[]( const msurface_t &a, const msurface_t &b )
			{
				// This is incredibly silly, but it works!
				return a.texinfo->material < b.texinfo->material;
			}
		);

		for ( msurface_t *surface = firstFace; surface < lastFace; ++surface )
		{
			R_BuildPolygonFromSurface( surface, endWorldIndices );
		}

		const uint32 lastWorldMesh = static_cast<uint32>( g_worldData.meshes.size() );
		subModel->numMeshes = lastWorldMesh - firstWorldMesh;
	}

	worldVertex_t *vertexData = g_worldData.vertices.data();
	const size_t vertexCount = g_worldData.vertices.size();
	const size_t vertexSize = vertexCount * sizeof( worldVertex_t );

	worldIndex_t *indexData = g_worldData.indices.data();
	const size_t indexCount = g_worldData.indices.size();
	const size_t indexSize = indexCount * sizeof( worldIndex_t );

#ifdef USE_MESHOPT

	for ( const worldMesh_t &mesh : g_worldData.meshes )
	{
		worldIndex_t *pOffsetIndices = indexData + mesh.firstIndex;

		meshopt_optimizeVertexCache(
			pOffsetIndices, pOffsetIndices,
			mesh.numIndices, vertexCount );

		// TODO: what do we gain from doing this?
#if 0
		meshopt_optimizeOverdraw(
			pOffsetIndices, pOffsetIndices,
			mesh.numIndices, (const float *)vertexData, TODO, sizeof( worldVertex_t ), 1.05f );
#endif
	}

	size_t numVertices = meshopt_optimizeVertexFetch(
		vertexData, indexData, indexCount,
		vertexData, vertexCount, sizeof( worldVertex_t ) );

#endif

	// Stats
	Com_Printf(
		S_COLOR_GREEN "------------------ World Stats ------------------\n"
		S_COLOR_GREEN "Uploaded to GPU:\n"
		S_COLOR_GREEN " Number of vertices:     %'zu\n"
		S_COLOR_GREEN " Vertices size in bytes: %$zu\n"
		S_COLOR_GREEN "Not uploaded to GPU:\n"
		S_COLOR_GREEN " Number of indices:      %'zu\n"
		S_COLOR_GREEN " Indices size in bytes:  %$zu\n"
		S_COLOR_GREEN "-------------------------------------------------\n",
		vertexCount,
		vertexSize,
		indexCount,
		indexSize
	);

	//
	// Upload our big fat vertex buffer
	//
	R_UploadBuffers( model );
}

void R_EraseWorldLists()
{
	if ( g_worldData.initialised )
	{
		glDeleteVertexArrays( 1, &g_worldData.vao );
		glDeleteBuffers( 2, &g_worldData.ebo );

		g_worldData.vertices.clear();
		g_worldData.indices.clear();
		g_worldData.lastMaterial = nullptr;
		g_worldData.meshes.clear();

		g_worldData.initialised = false;
	}
}

/*
===================================================================================================

	Wavefront OBJ Output

===================================================================================================
*/

static bool R_FindIndexInVector( worldIndex_t index, const std::vector<worldIndex_t> &checkedIndices )
{
	for ( const worldIndex_t checkIndex : checkedIndices )
	{
		if ( checkIndex == index )
		{
			return true;
		}
	}

	return false;
}

static void R_ExportRenderData( fsHandle_t file, const uint32 firstIndex, const uint32 lastIndex, const uint32 pass, const std::vector<worldVertex_t> &vertices )
{
	assert( !( lastIndex % 3 ) );

	const uint32 numIndices = lastIndex - firstIndex;

	std::vector<worldIndex_t> checkedIndices;
	checkedIndices.reserve( numIndices );

	if ( pass == 0 )
	{
		for ( uint32 iIndex = firstIndex; iIndex < lastIndex; ++iIndex )
		{
			// Print vertices
			const worldIndex_t index = g_worldData.indices[iIndex];

			if ( R_FindIndexInVector( index, checkedIndices ) )
			{
				continue;
			}

			checkedIndices.push_back( index );

			const worldVertex_t &vertex = vertices[index];

			FileSystem::PrintFileFmt( file,
				"v %g %g %g\n"
				"vt %g %g\n"
				"vn %g %g %g\n",
				vertex.pos[0], vertex.pos[1], vertex.pos[2],
				vertex.st2[0], vertex.st2[1],
				vertex.normal[0], vertex.normal[1], vertex.normal[2]
			);
		}
	}
	else
	{
		for ( uint32 iIndex = firstIndex; iIndex < lastIndex; iIndex += 3 )
		{
			const worldIndex_t index1 = g_worldData.indices[iIndex + 0] + 1;
			const worldIndex_t index2 = g_worldData.indices[iIndex + 1] + 1;
			const worldIndex_t index3 = g_worldData.indices[iIndex + 2] + 1;

			// Print indices
			FileSystem::PrintFileFmt( file,
				"f %d/%d/%d %d/%d/%d %d/%d/%d\n",
				index1, index1, index1,
				index2, index2, index2,
				index3, index3, index3 );
		}
	}
}

//
// Outputs world/<mapname>.obj
//
static void R_ExportWorldAsOBJ( const model_t *model, const std::vector<worldVertex_t> &vertices )
{
	char fileBase[MAX_QPATH];
	COM_FileBase( model->name, fileBase );

	char filename[MAX_QPATH];
	Q_sprintf_s( filename, "world/%s.obj", fileBase );

	fsHandle_t file = FileSystem::OpenFileWrite( filename, FS_GAMEDIR );
	if ( !file )
	{
		return;
	}

	Com_Printf( "Starting export of %s as an OBJ, please wait...\n", fileBase );

	//uint32 firstVertex = 0;

	for ( uint32 iPass = 0; iPass <= 1; ++iPass )
	{
		// The world is special and doesn't have a mesh definition because it's a waste of memory
		// to have one due to how we draw it
		const uint32 firstWorldIndex = model->submodels[0].firstMesh;
		const uint32 lastWorldIndex = firstWorldIndex + model->submodels[0].numMeshes;

		if ( iPass != 0 )
		{
			FileSystem::PrintFile(
				"o world\n"
				"s off\n",
				file );
		}

		R_ExportRenderData( file, firstWorldIndex, lastWorldIndex, iPass, vertices );

		const uint32 numSubModels = static_cast<uint32>( model->numsubmodels );

		for ( uint32 iSubModel = 1; iSubModel < numSubModels; ++iSubModel )
		{
			const mmodel_t *subModel = model->submodels + iSubModel;

			const uint32 firstMesh = subModel->firstMesh;
			const uint32 lastMesh = firstMesh + subModel->numMeshes;

			if ( iPass != 0 )
			{
				FileSystem::PrintFileFmt( file,
					"o submodel%03u\n"
					"s off\n",
					iSubModel );
			}

			for ( uint32 iMesh = firstMesh; iMesh < lastMesh; ++iMesh )
			{
				const worldMesh_t &mesh = g_worldData.meshes[iMesh];

				const uint32 firstIndex = mesh.firstIndex;
				const uint32 lastIndex = firstIndex + mesh.numIndices;

				R_ExportRenderData( file, firstIndex, lastIndex, iPass, vertices );
			}
		}
	}

	FileSystem::CloseFile( file );

	Com_Printf( "Exported %s to %s!\n", fileBase, filename );
}

static uint32 R_DumpWorldThreadProc( void *params )
{
	R_ExportWorldAsOBJ( r_worldmodel, g_worldData.vertices );

	return 0;
}

CON_COMMAND( r_dumpWorld, "Exports the world to an OBJ file in the maps folder.", 0 )
{
	if ( !r_worldmodel )
	{
		Com_Print( S_COLOR_YELLOW "No map loaded, can't dump the world\n" );
		return;
	}

	// fire and forget
	threadHandle_t thread = Sys_CreateThread( R_DumpWorldThreadProc, nullptr, THREAD_NORMAL, PLATTEXT( "Dump World Thread" ) );
	Sys_DestroyThread( thread );
}

/*
===================================================================================================

	BspExt Support

===================================================================================================
*/

static_assert( sizeof( bspDrawVert_t ) == sizeof( worldVertex_t ) );
static_assert( sizeof( bspDrawIndex_t ) == sizeof( worldIndex_t ) );

static void BspExt_GetBspExtName( const char *bspName, char *bspExtName, strlen_t maxLen )
{
	char strippedName[MAX_QPATH];
	COM_StripExtension( bspName, strippedName );

	Q_sprintf_s( bspExtName, maxLen, "%s.bspext", strippedName );
}

static void BspExt_LoadVertices( byte *base, bspExtLump_t *l )
{
	bspDrawVert_t *		in;
	worldVertex_t *		out;
	uint32				count;

	in = (bspDrawVert_t *)( base + l->fileofs );
	if ( l->filelen % sizeof( bspDrawVert_t ) )
	{
		Com_Error( "BspExt_Load: Funny lump size" );
	}
	count = l->filelen / sizeof( bspDrawVert_t );

	g_worldData.vertices.resize( count );

	out = g_worldData.vertices.data();

	// Identical binary structures, just do a memcpy
	memcpy( out, in, l->filelen );
}

static void BspExt_LoadIndices( byte *base, bspExtLump_t *l )
{
	bspDrawIndex_t *	in;
	worldIndex_t *		out;
	uint32				count;

	in = (bspDrawIndex_t *)( base + l->fileofs );
	if ( l->filelen % sizeof( bspDrawIndex_t ) )
	{
		Com_Error( "BspExt_Load: Funny lump size" );
	}
	count = l->filelen / sizeof( bspDrawIndex_t );

	g_worldData.indices.resize( count );

	out = g_worldData.indices.data();

	// Identical binary structures, just do a memcpy
	memcpy( out, in, l->filelen );
}

static void BspExt_LoadMeshes( const model_t *worldModel, byte *base, bspExtLump_t *l )
{
	bspDrawMesh_t *		in;
	worldMesh_t *		out;
	uint32				count;

	in = (bspDrawMesh_t *)( base + l->fileofs );
	if ( l->filelen % sizeof( bspDrawMesh_t ) )
	{
		Com_Error( "BspExt_Load: Funny lump size" );
	}
	count = l->filelen / sizeof( bspDrawMesh_t );

	g_worldData.meshes.resize( count );

	out = g_worldData.meshes.data();

	for ( uint32 i = 0; i < count; ++i )
	{
		out[i].texinfo = worldModel->texinfo + in->texinfo;
		out[i].firstIndex = in->firstIndex;
		out[i].numIndices = in->numIndices;
	}
}

static void BspExt_LoadModelsExt( const model_t *worldModel, byte *base, bspExtLump_t *l )
{
	bspModelExt_t *		in;
	mmodel_t *			out;
	uint32				count;

	in = (bspModelExt_t *)( base + l->fileofs );
	if ( l->filelen % sizeof( bspModelExt_t ) )
	{
		Com_Error( "BspExt_Load: Funny lump size" );
	}
	count = l->filelen / sizeof( bspModelExt_t );

	assert( count == worldModel->numsubmodels );

	out = worldModel->submodels;

	for ( uint32 i = 0; i < count; ++i )
	{
		out[i].firstMesh = in[i].firstMesh;
		out[i].numMeshes = in[i].numMeshes;
	}
}

static void BspExt_LoadFacesExt( const model_t *worldModel, byte *base, bspExtLump_t *l )
{
	bspFaceExt_t *		in;
	msurface_t *		out;
	uint32				count;

	in = (bspFaceExt_t *)( base + l->fileofs );
	if ( l->filelen % sizeof( bspFaceExt_t ) )
	{
		Com_Error( "BspExt_Load: Funny lump size" );
	}
	count = l->filelen / sizeof( bspFaceExt_t );

	assert( count == worldModel->numsurfaces );

	out = worldModel->surfaces;

	for ( uint32 i = 0; i < count; ++i )
	{
		out[i].firstIndex = in[i].firstIndex;
		out[i].numIndices = in[i].numIndices;
	}
}

static void BspExt_SortSurfacesByMaterial( const model_t *worldModel )
{
	const mmodel_t *firstModel = worldModel->submodels + 1; // Skip world
	const mmodel_t *endModel = worldModel->submodels + worldModel->numsubmodels; // Not a valid submodel

	for ( const mmodel_t *subModel = firstModel; subModel < endModel; ++subModel )
	{
		msurface_t *firstFace = worldModel->surfaces + subModel->firstface;
		msurface_t *lastFace = firstFace + subModel->numfaces;

		// Sort the surfaces by model, by material
		std::sort( firstFace, lastFace,
			[]( const msurface_t &a, const msurface_t &b )
			{
				// This is incredibly silly, but it works!
				return a.texinfo->material < b.texinfo->material;
			}
		);
	}
}

static void BspExt_UnSortSurfacesByMaterial( msurface_t *unsortedSurfaces, const model_t *worldModel )
{
	const mmodel_t *firstModel = worldModel->submodels + 1; // Skip world
	const mmodel_t *endModel = worldModel->submodels + worldModel->numsubmodels; // Not a valid submodel

	for ( const mmodel_t *subModel = firstModel; subModel < endModel; ++subModel )
	{
		msurface_t *firstFace = unsortedSurfaces + subModel->firstface;
		msurface_t *lastFace = firstFace + subModel->numfaces;

		// Sort the surfaces by model, by material
		std::sort( firstFace, lastFace,
			[]( const msurface_t &a, const msurface_t &b )
			{
				// This is incredibly silly, but it works!
				return a.bspFaceIndex < b.bspFaceIndex;
			}
		);
	}
}

bool BspExt_Load( model_t *worldModel )
{
	char bspExtName[MAX_QPATH];
	BspExt_GetBspExtName( worldModel->name, bspExtName, sizeof( bspExtName ) );

	byte *buffer;
	fsSize_t bspExtSize = FileSystem::LoadFile( bspExtName, (void **)&buffer );
	if ( !buffer )
	{
		return false;
	}

	if ( bspExtSize <= sizeof( bspExtHeader_t ) )
	{
		FileSystem::FreeFile( buffer );
		return false;
	}

	bspExtHeader_t *hdr = (bspExtHeader_t *)buffer;

	if ( hdr->ident != BSPEXT_IDENT || hdr->version != BSPEXT_VERSION )
	{
		FileSystem::FreeFile( buffer );
		return false;
	}

	worldModel->flags = hdr->flags;

	BspExt_LoadVertices( buffer, hdr->lumps + LUMP_DRAWVERTICES );
	BspExt_LoadIndices( buffer, hdr->lumps + LUMP_DRAWINDICES );

	BspExt_LoadMeshes( worldModel, buffer, hdr->lumps + LUMP_DRAWMESHES );
	BspExt_LoadModelsExt( worldModel, buffer, hdr->lumps + LUMP_MODELS_EXT );
	BspExt_LoadFacesExt( worldModel, buffer, hdr->lumps + LUMP_FACES_EXT );

	FileSystem::FreeFile( buffer );

	BspExt_SortSurfacesByMaterial( worldModel );

	R_UploadBuffers( worldModel );

	if ( worldModel->flags & BSPFLAG_EXTERNAL_LIGHTMAP )
	{
		R_LoadExternalLightmap( worldModel->name );
	}

	return true;
}

static void BspExt_SaveHeader( fsHandle_t handle, const model_t *worldModel, uint32 flags )
{
	bspExtHeader_t hdr;
	hdr.ident = BSPEXT_IDENT;
	hdr.version = BSPEXT_VERSION;
	hdr.flags = flags;

	uint32 offset = sizeof( bspExtHeader_t );

	const uint32 drawVerticesLen = g_worldData.vertices.size() * sizeof( bspDrawVert_t );
	hdr.lumps[LUMP_DRAWVERTICES].fileofs = offset;
	hdr.lumps[LUMP_DRAWVERTICES].filelen = drawVerticesLen;
	offset += drawVerticesLen;

	const uint32 drawIndicesLen = g_worldData.indices.size() * sizeof( bspDrawIndex_t );
	hdr.lumps[LUMP_DRAWINDICES].fileofs = offset;
	hdr.lumps[LUMP_DRAWINDICES].filelen = drawIndicesLen;
	offset += drawIndicesLen;

	const uint32 drawMeshesLen = g_worldData.meshes.size() * sizeof( bspDrawMesh_t );
	hdr.lumps[LUMP_DRAWMESHES].fileofs = offset;
	hdr.lumps[LUMP_DRAWMESHES].filelen = drawMeshesLen;
	offset += drawMeshesLen;

	const uint32 modelsExtLen = worldModel->numsubmodels * sizeof( bspModelExt_t );
	hdr.lumps[LUMP_MODELS_EXT].fileofs = offset;
	hdr.lumps[LUMP_MODELS_EXT].filelen = modelsExtLen;
	offset += modelsExtLen;

	const uint32 facesExtLen = worldModel->numsurfaces * sizeof( bspFaceExt_t );
	hdr.lumps[LUMP_FACES_EXT].fileofs = offset;
	hdr.lumps[LUMP_FACES_EXT].filelen = facesExtLen;
	offset += facesExtLen;

	FileSystem::WriteFile( &hdr, sizeof( hdr ), handle );
}

static void BspExt_SaveMeshes( fsHandle_t handle, const model_t *worldModel )
{
	bspDrawMesh_t out;

	const worldMesh_t *meshes = g_worldData.meshes.data();
	const uint32 numMeshes = g_worldData.meshes.size();

	// NOTE: Could batch this but it really doesn't matter as long as it ends up in the file
	for ( uint32 i = 0; i < numMeshes; ++i )
	{
		out.texinfo = static_cast<uint16>( meshes[i].texinfo - worldModel->texinfo );
		out.firstIndex = meshes[i].firstIndex;
		out.numIndices = meshes[i].numIndices;

		FileSystem::WriteFile( &out, sizeof( out ), handle );
	}
}

static void BspExt_SaveModelsExt( fsHandle_t handle, const model_t *worldModel )
{
	bspModelExt_t out;

	const mmodel_t *models = worldModel->submodels;
	const uint32 numModels = worldModel->numsubmodels;

	for ( uint32 i = 0; i < numModels; ++i )
	{
		out.firstMesh = models[i].firstMesh;
		out.numMeshes = models[i].numMeshes;

		FileSystem::WriteFile( &out, sizeof( out ), handle );
	}
}

static void BspExt_SaveFacesExt( fsHandle_t handle, const model_t *worldModel )
{
	bspFaceExt_t out;

	const msurface_t *surfs = worldModel->surfaces;
	const uint32 numSurfs = worldModel->numsurfaces;

	// Unsort our surfaces like a maniac
	std::vector<msurface_t> unsortedSurfaces( numSurfs );
	memcpy( unsortedSurfaces.data(), surfs, numSurfs * sizeof( msurface_t ) );

	BspExt_UnSortSurfacesByMaterial( unsortedSurfaces.data(), worldModel );

	surfs = unsortedSurfaces.data();

	for ( uint32 i = 0; i < numSurfs; ++i )
	{
		out.firstIndex = surfs[i].firstIndex;
		out.numIndices = surfs[i].numIndices;

		FileSystem::WriteFile( &out, sizeof( out ), handle );
	}
}

void BspExt_Save( const model_t *worldModel, uint32 flags )
{
	char bspExtName[MAX_QPATH];
	BspExt_GetBspExtName( worldModel->name, bspExtName, sizeof( bspExtName ) );

	fsHandle_t handle = FileSystem::OpenFileWrite( bspExtName, FS_GAMEDIR );
	if ( !handle )
	{
		Com_Print( S_COLOR_YELLOW "Failed to write bspext\n" );
		return;
	}

	BspExt_SaveHeader( handle, worldModel, flags );

	//
	// Vertices
	//
	if ( flags & BSPFLAG_EXTERNAL_LIGHTMAP )
	{
		// THIS IS SLOW SLOW SLOW!!!

		Com_Print( "Atlasing world, this may take a while...\n" );

		std::vector<worldVertex_t> atlasVertices = g_worldData.vertices;

		R_AtlasWorldLists( worldModel->name, atlasVertices.data(), g_worldData.indices.data(), atlasVertices.size(), g_worldData.indices.size() );

		Com_Print( "Atlasing complete!\n" );

		FileSystem::WriteFile( atlasVertices.data(), atlasVertices.size() * sizeof( worldVertex_t ), handle );
	}
	else
	{
		FileSystem::WriteFile( g_worldData.vertices.data(), g_worldData.vertices.size() * sizeof( worldVertex_t ), handle );
	}
	
	//
	// Indices
	//
	FileSystem::WriteFile( g_worldData.indices.data(), g_worldData.indices.size() * sizeof( worldIndex_t ), handle );

	BspExt_SaveMeshes( handle, worldModel );
	BspExt_SaveModelsExt( handle, worldModel );
	BspExt_SaveFacesExt( handle, worldModel );

	FileSystem::CloseFile( handle );
}

static uint32 R_WriteBSPExtThreadProc( void *params )
{
	uint32 *flags = (uint32 *)params;

	BspExt_Save( r_worldmodel, *flags );

	Mem_Free( params );

	return 0;
}

CON_COMMAND( r_writeBSPExt, "Writes out the bspext file for the loaded bsp.", 0 )
{
	if ( !r_worldmodel )
	{
		Com_Print( S_COLOR_YELLOW "No map loaded, can't write bspext\n" );
		return;
	}

	uint32 flags = 0;

	const int argc = Cmd_Argc();
	for ( int i = 0; i < argc; ++i )
	{
		const char *arg = Cmd_Argv( i );

		if ( Q_strcmp( arg, "--atlas" ) == 0 )
		{
			flags |= BSPFLAG_EXTERNAL_LIGHTMAP;
			continue;
		}
	}

	uint32 *params = (uint32 *)Mem_Alloc( sizeof( uint32 ) );
	*params = flags;

	// fire and forget
	threadHandle_t thread = Sys_CreateThread( R_WriteBSPExtThreadProc, params, THREAD_NORMAL, PLATTEXT( "BSPEXT Writer Thread" ) );
	Sys_DestroyThread( thread );
}

/*
===================================================================================================
===================================================================================================
*/


