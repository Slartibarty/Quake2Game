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

#include "../../core/threading.h"

#include "../shared/imgtools.h"
#include <algorithm> // std::sort
#include <vector>

// Comment out to disable meshoptimizer optimisations
// TODO: Reconsider how this can be used
//#define USE_MESHOPT

// Experimental atlasing stuff
//#define USE_XATLAS

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

// Must be contiguous to send to OpenGL
struct worldVertex_t
{
	vec3_t	pos;
	float	st1[2];		// Normal UVs
	float	st2[2];		// Lightmap UVs
	vec3_t	normal;
};

using worldIndex_t = uint32; // Should really be toggled between based on the vertex count of the map...

// A surface batch
struct worldMesh_t
{
	mtexinfo_t *	texinfo;
	uint32			firstIndex;		// Index into s_worldLists.finalIndices
	uint32			numIndices;
	//uint32		numVertices;	// For meshoptimizer
};

// This is the data sent to OpenGL
// We use a single, large vertex / index buffer
// for all world geometry
// Surfaces store indices into the index buffer
struct worldRenderData_t
{
	std::vector<worldVertex_t>	vertices;
	std::vector<worldIndex_t>	indices;

	material_t *				lastMaterial;		// This is used when building the vector below
	std::vector<worldMesh_t>	meshes;

	GLuint vao, vbo, ebo;
	//GLuint eboCluster;		// ebo for the current viscluster

	bool initialised;
};

static worldRenderData_t s_worldRenderData;

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

static vec3_t		modelorg;		// relative to viewpoint

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

//static GLuint fuckedUpLightmapID;

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
	GL_BindTexture( gl_lms.lightmapTextures[1] );
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
	int			i;
	bool		rotated;

	if ( currentmodel->numMeshes == 0 )
	{
		return;
	}

	currententity = e;

	if ( e->angles[0] || e->angles[1] || e->angles[2] )
	{
		rotated = true;
		for ( i = 0; i < 3; ++i )
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

	if ( R_CullBox( mins, maxs ) )
	{
		return;
	}

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

	XMMATRIX modelMatrix = XMMatrixMultiply(
		XMMatrixRotationX( DEG2RAD( e->angles[ROLL] ) ) * XMMatrixRotationY( DEG2RAD( e->angles[PITCH] ) ) * XMMatrixRotationZ( DEG2RAD( e->angles[YAW] ) ),
		XMMatrixTranslation( e->origin[0], e->origin[1], e->origin[2] )
	);

	XMFLOAT4X4A modelMatrixStore;
	XMStoreFloat4x4A( &modelMatrixStore, modelMatrix );

	glBindVertexArray( s_worldRenderData.vao );
	glBindBuffer( GL_ARRAY_BUFFER, s_worldRenderData.vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, s_worldRenderData.ebo );

	GL_UseProgram( glProgs.worldProg );

	//glUniformMatrix4fv( 4, 1, GL_FALSE, (const GLfloat *)&modelMatrixStore );

	worldMesh_t *firstMesh = s_worldRenderData.meshes.data() + currentmodel->firstMesh;
	worldMesh_t *lastMesh = firstMesh + currentmodel->numMeshes;

	for ( worldMesh_t *mesh = firstMesh; mesh < lastMesh; ++mesh )
	{
		R_DrawWorldMesh( *mesh );
	}

	GL_UseProgram( 0 );
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
	const auto begin = s_worldRenderData.indices.begin() + surf->firstIndex;
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
		const worldVertex_t &vertex = s_worldRenderData.vertices[s_worldRenderData.indices[i]];
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
			if ( tr.refdef.areabits )
			{
				if ( !( tr.refdef.areabits[leaf->area >> 3] & ( 1 << ( leaf->area & 7 ) ) ) )
				{
					// not visible
					return;
				}
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

		msurface_t *firstSurface = r_worldmodel->surfaces + node->firstsurface;
		msurface_t *lastSurface = firstSurface + node->numsurfaces;

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

	glBindVertexArray( s_worldRenderData.vao );
	glBindBuffer( GL_ARRAY_BUFFER, s_worldRenderData.vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, s_worldRenderData.ebo );

	// Build the world lists

	worldLists_t worldLists;

	{
		worldNodeWork_t work;

		// Determine which leaves are in the PVS / areamask
		R_MarkLeaves();

		// This figures out what we need to render and builds the world lists
		R_RecursiveWorldNode( work, r_worldmodel->nodes );

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

	//R_DrawSkyBox();

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

//
// Given an msurface_t with basic brush geometry specified
// will create optimised render geometry and append it to s_worldRenderData
// It then gives the index of the first renderindex to the msurface_t
//
static void R_BuildPolygonFromSurface( msurface_t *fa, bool world )
{
	ZoneScoped

	const int *pSurfEdges = currentmodel->surfedges;
	const medge_t *pEdges = currentmodel->edges;
	const mvertex_t *pVertices = currentmodel->vertexes;
	const int numEdges = fa->numedges; // Equal to the number of vertices

	// Index of the first vertex
	const uint32 firstVertex = static_cast<uint32>( s_worldRenderData.vertices.size() );

	// Reconstruct the polygon

	// First, build the vertices for this face

	for ( int i = 0; i < numEdges; ++i )
	{
		const int surfEdge = pSurfEdges[fa->firstedge + i];
		const int startIndex = surfEdge > 0 ? 0 : 1;

		const medge_t *pEdge = pEdges + abs( surfEdge );
		const float *pVertex = pVertices[pEdge->v[startIndex]].position;

		worldVertex_t &outVertex = s_worldRenderData.vertices.emplace_back();

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
	//const uint32 endVertex = static_cast<uint32>( s_worldRenderData.vertices.size() );
	//const uint32 numVertices = endVertex - firstVertex;

	// Index of the first index!
	const uint32 firstIndex = static_cast<uint32>( s_worldRenderData.indices.size() );

	// Triangulate it, fan style

	const int numTriangles = numEdges - 2;

	// For every triangle
	for ( int i = 0; i < numTriangles; ++i )
	{
		s_worldRenderData.indices.push_back( firstVertex + 0 );
		s_worldRenderData.indices.push_back( firstVertex + i + 1 );
		s_worldRenderData.indices.push_back( firstVertex + i + 2 );
	}

	// Not a valid index
	const uint32 endIndex = static_cast<uint32>( s_worldRenderData.indices.size() );
	const uint32 numIndices = endIndex - firstIndex;

	fa->firstIndex = firstIndex;
	fa->numIndices = numIndices;

	assert( !( fa->numIndices % 3 ) );

	//
	// Figure out where we're creating the mesh
	//
	if ( !world )
	{
		if ( s_worldRenderData.lastMaterial != fa->texinfo->material )
		{
			s_worldRenderData.lastMaterial = fa->texinfo->material;

			// Create a new mesh entry
			worldMesh_t &mesh = s_worldRenderData.meshes.emplace_back();

			mesh.texinfo = fa->texinfo;
			mesh.firstIndex = firstIndex;
			mesh.numIndices = numIndices;
		}
		else
		{
			// Append to our existing one
			worldMesh_t &mesh = s_worldRenderData.meshes.back();
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

	//constexpr size_t offsetNormals = offsetof( worldVertex_t, normal );
	//constexpr size_t offsetUVs = offsetof( worldVertex_t, st1 );

	const xatlas::MeshDecl meshDecl
	{
		.vertexPositionData = vertices,
		//.vertexNormalData = (byte *)vertices + offsetNormals,
		//.vertexUvData = (byte *)vertices + offsetUVs,
		.indexData = indices,

		.vertexCount = numVertices,
		.vertexPositionStride = sizeof( worldVertex_t ),
		//.vertexNormalStride = sizeof( worldVertex_t ),
		//.vertexUvStride = sizeof( worldVertex_t ),
		.indexCount = numIndices,
		.indexFormat = xatlas::IndexFormat::UInt32
	};

	xatlas::AddMeshError meshError = xatlas::AddMesh( pAtlas, meshDecl, 1 );
	if ( meshError != xatlas::AddMeshError::Success ) {
		xatlas::Destroy( pAtlas );
		Com_Printf( "XAtlas failed: %s\n", xatlas::StringForEnum( meshError ) );
		return;
	}

	xatlas::ChartOptions chartOptions
	{
		.maxIterations = 1
	};

	xatlas::PackOptions packOptions
	{
		.bruteForce = false
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
			myVertex->st2[1] = 1.0f - vertex.uv[1] / pAtlas->height;
		}
	}

#ifdef USE_XATLAS

#if 1

	byte *pBuffer;
	fsSize_t nBufLen = FileSystem::LoadFile( "world/test_biggerbox.png", (void **)&pBuffer );
	if ( !pBuffer )
	{
		return;
	}

	assert( nBufLen > 32 ); // Sanity check

	if ( img::TestPNG( pBuffer ) )
	{
		int width, height;
		byte *pPic = img::LoadPNG( pBuffer, width, height );
		FileSystem::FreeFile( pBuffer );

		glGenTextures( 1, &fuckedUpLightmapID );
		GL_ActiveTexture( GL_TEXTURE1 );
		GL_BindTexture( fuckedUpLightmapID );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexImage2D( GL_TEXTURE_2D,
			0,
			GL_LIGHTMAP_INTERNAL_FORMAT,
			width, height,
			0,
			GL_LIGHTMAP_FORMAT,
			GL_UNSIGNED_BYTE,
			pPic );

		Mem_Free( pPic );
	}

#else

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
			FileSystem::PrintFileFmt( file, "v %g %g %g\n", myVertex->pos[0], myVertex->pos[1], myVertex->pos[2]);
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

#endif
}

void R_BuildWorldLists( model_t *model )
{
	ZoneScoped

	//
	// Reserve at least enough space to hold all the base map vertices
	// The reserve amount is a rough approximation of what a map will need,
	// not what it will actually use
	//
	s_worldRenderData.vertices.reserve( model->numvertexes );
	s_worldRenderData.indices.reserve( model->numvertexes * 3 );

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
			R_BuildPolygonFromSurface( surface, true );
		}
	}

	// Where the world indices end
	const uint32 endWorldIndices = static_cast<uint32>( s_worldRenderData.indices.size() );

	// Store the number of indices in the mesh stuff, for r_dumpworld
	worldModel->firstMesh = 0;
	worldModel->numMeshes = endWorldIndices;

	mmodel_t *firstModel = model->submodels + 1;
	mmodel_t *endModel = model->submodels + model->numsubmodels; // Not a valid submodel

	for ( mmodel_t *subModel = firstModel; subModel < endModel; ++subModel )
	{
		const uint32 firstWorldMesh = static_cast<uint32>( s_worldRenderData.meshes.size() );
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
			R_BuildPolygonFromSurface( surface, false );
		}

		const uint32 lastWorldMesh = static_cast<uint32>( s_worldRenderData.meshes.size() );
		subModel->numMeshes = lastWorldMesh - firstWorldMesh;
	}

	worldVertex_t *vertexData = s_worldRenderData.vertices.data();
	const size_t vertexCount = s_worldRenderData.vertices.size();
	const size_t vertexSize = vertexCount * sizeof( worldVertex_t );

	worldIndex_t *indexData = s_worldRenderData.indices.data();
	const size_t indexCount = s_worldRenderData.indices.size();
	const size_t indexSize = indexCount * sizeof( worldIndex_t );

#ifdef USE_MESHOPT

	for ( const worldMesh_t &mesh : s_worldRenderData.meshes )
	{
		worldIndex_t *pOffsetIndices = indexData + mesh.firstIndex;

		meshopt_optimizeVertexCache(
			pOffsetIndices, pOffsetIndices,
			mesh.numIndices, vertexCount );

		// TODO: what do we gain from doing this?
		meshopt_optimizeOverdraw(
			pOffsetIndices, pOffsetIndices,
			mesh.numIndices, (const float *)vertexData, vertexCount, sizeof( worldVertex_t ), 1.05f );
	}

	size_t numVertices = meshopt_optimizeVertexFetch(
		vertexData, indexData, indexCount,
		vertexData, vertexCount, sizeof( worldVertex_t ) );

#endif

	R_AtlasWorldLists( model->name, vertexData, indexData, vertexCount, indexCount );

	//
	// Upload our big fat vertex buffer
	//

	glGenVertexArrays( 1, &s_worldRenderData.vao );
	glGenBuffers( 2, &s_worldRenderData.vbo );			// Gen the vbo and ebo in one go

	glBindVertexArray( s_worldRenderData.vao );
	glBindBuffer( GL_ARRAY_BUFFER, s_worldRenderData.vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, s_worldRenderData.ebo );

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
	//glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexSize, indexData, GL_STATIC_DRAW );

	s_worldRenderData.initialised = true;
}

void R_EraseWorldLists()
{
	if ( s_worldRenderData.initialised )
	{
		glDeleteVertexArrays( 1, &s_worldRenderData.vao );
		glDeleteBuffers( 2, &s_worldRenderData.ebo );

		s_worldRenderData.vertices.clear();
		s_worldRenderData.indices.clear();
		s_worldRenderData.lastMaterial = nullptr;
		s_worldRenderData.meshes.clear();

		s_worldRenderData.initialised = false;
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

static void R_ExportRenderData( fsHandle_t file, const uint32 firstIndex, const uint32 lastIndex, const uint32 pass )
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
			const worldIndex_t index = s_worldRenderData.indices[iIndex];

			if ( R_FindIndexInVector( index, checkedIndices ) )
			{
				continue;
			}

			checkedIndices.push_back( index );

			const worldVertex_t &vertex = s_worldRenderData.vertices[index];

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
			const worldIndex_t index1 = s_worldRenderData.indices[iIndex + 0] + 1;
			const worldIndex_t index2 = s_worldRenderData.indices[iIndex + 1] + 1;
			const worldIndex_t index3 = s_worldRenderData.indices[iIndex + 2] + 1;

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
static void R_ExportWorldAsOBJ( const model_t *model )
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

		R_ExportRenderData( file, firstWorldIndex, lastWorldIndex, iPass );

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
				const worldMesh_t &mesh = s_worldRenderData.meshes[iMesh];

				const uint32 firstIndex = mesh.firstIndex;
				const uint32 lastIndex = firstIndex + mesh.numIndices;

				R_ExportRenderData( file, firstIndex, lastIndex, iPass );
			}
		}
	}

	FileSystem::CloseFile( file );

	Com_Printf( "Exported %s to %s!\n", fileBase, filename );
}

static uint32 R_DumpWorldThreadProc( void *params )
{
	R_ExportWorldAsOBJ( r_worldmodel );

	return 0;
}

CON_COMMAND( r_dumpworld, "Exports the world to an OBJ file in the maps folder.", 0 )
{
	if ( !r_worldmodel )
	{
		Com_Print( S_COLOR_YELLOW "No map loaded, can't dump the world\n" );
		return;
	}

	// fire and forget
	threadHandle_t thread = Sys_CreateThread( R_DumpWorldThreadProc, nullptr, THREAD_NORMAL, PLATTEXT( "Dump World Thread" ), CORE_ANY );
	Sys_DestroyThread( thread );
}
