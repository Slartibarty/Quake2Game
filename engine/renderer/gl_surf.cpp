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

// Use the PVS to determine what faces are visible, broken right now
//#define USE_PVS

// Comment out to disable meshoptimizer optimisations
#define USE_MESHOPT

#ifdef USE_MESHOPT
#define MESHOPTIMIZER_NO_WRAPPERS
#include "../../thirdparty/meshoptimizer/src/meshoptimizer.h"
#endif

#define MAX_SHADERLIGHTS	4

#define DYNAMIC_LIGHT_WIDTH		128
#define DYNAMIC_LIGHT_HEIGHT	128

#define LIGHTMAP_BYTES		4

// Width and height of the lightmap atlas
#define	BLOCK_WIDTH		1024
#define	BLOCK_HEIGHT	1024

// How many lightmap atlases we can have
#define	MAX_LIGHTMAPS	4

#define GL_LIGHTMAP_FORMAT GL_RGBA

#define TEXNUM_LIGHTMAPS	1024

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
};

// This is the data sent to OpenGL
// We use a single, large vertex / index buffer
// for all world geometry
// Surfaces store indices into the index buffer
struct worldRenderData_t
{
	std::vector<worldVertex_t>	vertices;
	std::vector<worldIndex_t>	indices;

#ifndef USE_PVS
	material_t *				lastMaterial;		// This is used when building the vector below
	std::vector<worldMesh_t>	meshes;
#endif

	GLuint vao, vbo/*, ebo*/;
	GLuint eboCluster;		// ebo for the current viscluster
};

static worldRenderData_t s_worldRenderData;

#ifdef USE_PVS

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

static worldLists_t s_worldLists;

#endif

static vec3_t		modelorg;		// relative to viewpoint

struct gllightmapstate_t
{
	int				current_lightmap_texture;

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

/*
===================================================================================================

	BRUSH MODELS

===================================================================================================
*/

//
// Returns the proper texture for a given time and base texture
//
static material_t *R_TextureAnimation( mtexinfo_t *tex )
{
	if ( !tex->next )
	{
		return tex->material;
	}

	int c = currententity->frame % tex->numframes;
	while (c)
	{
		tex = tex->next;
		c--;
	}

	return tex->material;
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
// Used by R_DrawBrushModel
//
static void R_DrawInlineBModel()
{
#if 0
	int			i, k;
	cplane_t	*pplane;
	float		dot;
	msurface_t	*psurf;
	dlight_t	*lt;

	// calculate dynamic lighting for bmodel
	if ( !r_flashblend->GetBool() )
	{
		lt = tr.refdef.dlights;
		for (k=0 ; k<tr.refdef.num_dlights ; k++, lt++)
		{
			R_MarkLights (lt, 1<<k, currentmodel->nodes + currentmodel->firstnode);
		}
	}

	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];

	if ( currententity->flags & RF_TRANSLUCENT )
	{
		glEnable (GL_BLEND);
		glColor4f (1,1,1,0.25f);
		GL_TexEnv( GL_MODULATE );
	}

	//
	// draw texture
	//
	for (i=0 ; i<currentmodel->nummodelsurfaces ; i++, psurf++)
	{
	// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;

	// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (psurf->texinfo->flags & (SURF_TRANS33|SURF_TRANS66) )
			{	// add to the translucent chain
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
			}
			else if ( ( GLEW_ARB_multitexture && r_ext_multitexture->GetBool() ) && !( psurf->flags & SURF_DRAWTURB ) )
			{
				R_RenderSurface( psurf );
			}
			else
			{
				GL_EnableMultitexture( false );
				R_RenderBrushPoly( psurf );
				GL_EnableMultitexture( true );
			}
		}
	}

	if ( currententity->flags & RF_TRANSLUCENT )
	{
		glDisable (GL_BLEND);
		glColor4f (1,1,1,1);
		GL_TexEnv( GL_REPLACE );
	}
#endif
}

//
// Returns true if the box is completely outside the frustum
//
static bool R_CullBox( vec3_t mins, vec3_t maxs )
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
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, s_worldRenderData.eboCluster );

	GL_UseProgram( glProgs.worldProg );

	glUniformMatrix4fv( 4, 1, GL_FALSE, (const GLfloat *)&modelMatrixStore );

	worldMesh_t *firstMesh = s_worldRenderData.meshes.data() + currentmodel->firstMesh;
	worldMesh_t *lastMesh = firstMesh + currentmodel->numMeshes;

	for ( worldMesh_t *mesh = firstMesh; mesh < lastMesh; ++mesh )
	{
		const material_t *mat = R_TextureAnimation( mesh->texinfo );

		GL_ActiveTexture( GL_TEXTURE0 );
		mat->Bind();
		GL_ActiveTexture( GL_TEXTURE1 );
		GL_BindTexture( glState.lightmap_textures + 1 );

		glDrawElements( GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, (void *)( (uintptr_t)( mesh->firstIndex ) * sizeof( worldIndex_t ) ) );
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
}

/*
===================================================================================================

	WORLD MODEL

===================================================================================================
*/

#ifdef USE_PVS

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

		// Check for door connected areas
		if ( tr.refdef.areabits )
		{
			if ( !( tr.refdef.areabits[leaf->area >> 3] & ( 1 << ( leaf->area & 7 ) ) ) )
			{
				// not visible
				continue;
			}
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

static void R_AddSurfaceToMaterialSet( msurface_t *surf, worldMaterialSet_t &materialSet )
{
	const auto begin = s_worldRenderData.indices.begin() + surf->firstIndex;
	const auto end = begin + surf->numIndices;

	materialSet.indices.insert( materialSet.indices.end(), begin, end );

	assert( !( materialSet.indices.size() % 3 ) );
}

static void R_AddSurface( msurface_t *surf, worldNodeWork_t &work )
{
#if 0
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
	for ( uint i = surf->firstIndex; i < surf->firstIndex + surf->numIndices; ++i )
	{
		const worldVertex_t &vertex = s_worldRenderData.vertices[s_worldRenderData.indices[i]];
		glVertex3fv( (float *)&vertex.pos );
	}
	glEnd();
#endif
}

static void R_HandleLeaf( worldNodeWork_t &work, mleaf_t *leaf )
{
	const int firstMarkSurface = leaf->firstmarksurface;
	const int lastMarkSurface = firstMarkSurface + leaf->nummarksurfaces;

	int c;
	msurface_t *surf;

	for ( c = firstMarkSurface, surf = r_worldmodel->marksurfaces[firstMarkSurface]; c < lastMarkSurface; ++c, ++surf )
	{
		// Mark this surface to be drawn at the node
		surf->frameCount = tr.frameCount;
		R_AddSurface( surf, work );
	}
}

//
// Recurses down the BSP tree inserting surfaces that need to be drawn
// into the world lists from back to front (TODO: make it front to back)
//
static void R_RecursiveWorldNode( worldNodeWork_t &work, mnode_t *node )
{
	int				c, side, sidebit;
	cplane_t *		plane;
	msurface_t *	surf;
	float			dot;

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

		// Frustrum cull
		//if ( R_CullBox( node->minmaxs, node->minmaxs + 3 ) ) {
		//	return;
		//}

		// if a leaf node, draw stuff
		if ( node->contents != -1 )
		{
			R_HandleLeaf( work, (mleaf_t *)node );
			return;
		}

		// node is just a decision point, so go down the appropriate sides

		// find which side of the node we are on
		plane = node->plane;

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

		if ( dot >= 0 )
		{
			side = 0;
			sidebit = 0;
		}
		else
		{
			side = 1;
			sidebit = SURF_PLANEBACK;
		}

		// Recurse down the children, front side first
		R_RecursiveWorldNode( work, node->children[side] );

		// draw stuff
#if 1
		const int firstSurface = node->firstsurface;
		const int lastSurface = node->firstsurface + node->numsurfaces;

		for ( c = firstSurface, surf = r_worldmodel->surfaces + firstSurface; c < lastSurface; ++c, ++surf )		// Back to front
		{
			if ( surf->frameCount != tr.frameCount )
			{
				// Already visited
				continue;
			}

			if ( ( surf->flags & SURF_PLANEBACK ) != sidebit )
			{
				// Wrong side
				continue;
			}

			if ( surf->texinfo->flags & SURF_SKY )
			{
				// Just adds to visible sky bounds
				//R_AddSkySurface( surf );
			}
			else if ( surf->texinfo->flags & ( SURF_TRANS33 | SURF_TRANS66 ) )
			{
				// Remove?
			}
			else
			{
				R_AddSurface( surf, work );
			}
		}
#endif

		// Recurse down the back side
		// We can prevent a recursion by just using this while loop
		//R_RecursiveWorldNode( work, node->children[!side] );
		node = node->children[!side];
	}
}

//
// Squashes our index lists into the final index buffer, builds the world lists
//
static void R_SquashAndUploadIndices( const worldNodeWork_t &work )
{
#if 1
	for ( const worldMaterialSet_t &materialSet : work.materialSets )
	{
		// Create a new mesh
		worldMesh_t &opaqueMesh = s_worldLists.opaqueMeshes.emplace_back();
		opaqueMesh.texinfo = materialSet.texinfo;

		uint32 firstIndex = static_cast<uint32>( s_worldLists.finalIndices.size() );

		// Insert our individual index buffer at the end of the final index buffer
		s_worldLists.finalIndices.insert( s_worldLists.finalIndices.end(), materialSet.indices.begin(), materialSet.indices.end() );

		uint32 end = static_cast<uint32>( s_worldLists.finalIndices.size() );

		assert( end - firstIndex == materialSet.indices.size() );

		opaqueMesh.firstIndex = firstIndex;
		opaqueMesh.numIndices = materialSet.indices.size();
	}

	// Squashed! Now upload the index buffer
	//glBindVertexArray( s_worldRenderData.vao );
	//glBindBuffer( GL_ARRAY_BUFFER, s_worldRenderData.vbo );
	//glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, s_worldRenderData.eboCluster );

	const void *indexData = reinterpret_cast<const void *>( s_worldLists.finalIndices.data() );
	const GLsizeiptr indexSize = static_cast<GLsizeiptr>( s_worldLists.finalIndices.size() ) * sizeof( worldIndex_t );

	glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexSize, indexData, GL_DYNAMIC_DRAW );
#endif
}

#endif

//
// Draws all opaque surfaces in the world list
//
static void R_DrawStaticOpaqueWorld()
{
#ifdef USE_PVS
	for ( const worldMesh_t &mesh : s_worldLists.opaqueMeshes )
#else
	mmodel_t &model = r_worldmodel->submodels[0];

	// This will always be the first, but it's here for reference
	worldMesh_t *firstMesh = s_worldRenderData.meshes.data() + model.firstMesh;
	worldMesh_t *lastMesh = firstMesh + model.numMeshes;

	for ( worldMesh_t *mesh = firstMesh; mesh < lastMesh; ++mesh )
#endif
	{
		const material_t *mat = R_TextureAnimation( mesh->texinfo );

		GL_ActiveTexture( GL_TEXTURE0 );
		mat->Bind();
		GL_ActiveTexture( GL_TEXTURE1 );
		GL_BindTexture( glState.lightmap_textures + 1 );

		glDrawElements( GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, (void *)( (uintptr_t)( mesh->firstIndex ) * sizeof( worldIndex_t ) ) );
	}
}

//
// Main entry point for drawing static, opaque world geometry
//
void R_DrawWorld()
{
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

	glBindVertexArray( s_worldRenderData.vao );
	glBindBuffer( GL_ARRAY_BUFFER, s_worldRenderData.vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, s_worldRenderData.eboCluster );

	// Build the world lists
#ifdef USE_PVS
	{
		worldNodeWork_t work;

		s_worldLists.ClearAllLists();

		// Determine which leaves are in the PVS / areamask
		R_MarkLeaves();

		// This figures out what we need to render and builds the world lists
		R_RecursiveWorldNode( work, r_worldmodel->nodes );

		//R_SquashAndUploadIndices( work );
	}
#endif

#if 1

	DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixIdentity();

	DirectX::XMFLOAT4X4A modelMatrixStore;
	DirectX::XMStoreFloat4x4A( &modelMatrixStore, modelMatrix );

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

	// Now render stuff!
	R_DrawStaticOpaqueWorld();

	GL_UseProgram( 0 );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );

#endif

	// CLEAN UP STUFF!!!!!!!!!!!!!!!!!!!!!

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
	int texture = dynamic ? 0 : gl_lms.current_lightmap_texture;

	GL_ActiveTexture( GL_TEXTURE1 );
	GL_BindTexture( glState.lightmap_textures + texture );
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
			GL_RGBA8,
			BLOCK_WIDTH, BLOCK_HEIGHT,
			0,
			GL_LIGHTMAP_FORMAT,
			GL_UNSIGNED_BYTE,
			gl_lms.lightmap_buffer );

		++gl_lms.current_lightmap_texture;

		if ( gl_lms.current_lightmap_texture == MAX_LIGHTMAPS )
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

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

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

	if ( !glState.lightmap_textures )
	{
		glState.lightmap_textures = TEXNUM_LIGHTMAPS;
	}

	gl_lms.current_lightmap_texture = 1;

	/*
	** initialize the dynamic lightmap texture
	*/
	GL_BindTexture( glState.lightmap_textures + 0 );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D,
		0,
		GL_RGBA8,
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

// Populates the normal member of the vertex structures
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

//
// Given an msurface_t with basic brush geometry specified
// will create optimised render geometry and append it to s_worldRenderData
// It then gives the index of the first renderindex to the msurface_t
//
static void R_BuildPolygonFromSurface( msurface_t *fa, mmodel_t *world )
{
	const int *pSurfEdges = currentmodel->surfedges;
	const medge_t *pEdges = currentmodel->edges;
	const mvertex_t *pVertices = currentmodel->vertexes;
	const int numEdges = fa->numedges; // Equal to the number of vertices

	// Index of the first vertex
	const uint32 firstVertex = static_cast<uint32>( s_worldRenderData.vertices.size() );

	// Reconstruct the polygon

#if 0
	glpoly_t *poly = (glpoly_t *)Hunk_Alloc( sizeof( glpoly_t ) + ( numEdges - 4 ) * VERTEXSIZE * sizeof( float ) );
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = numEdges;
#endif

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

		// TODO: This isn't the best solution, the plane normals can be reversed sometimes
		// which breaks the shader
		VectorCopy( fa->plane->normal, outVertex.normal );
	}

	// index of the first index!
	const uint32 firstIndex = static_cast<uint32>( s_worldRenderData.indices.size() );

	// triangulate it, fan style

	const int numTriangles = numEdges - 2;

	// for every triangle
	for ( int i = 0; i < numTriangles; ++i )
	{
#if 0
		worldVertex_t &v1 = s_worldRenderData.vertices[firstVertex + 0];
		worldVertex_t &v2 = s_worldRenderData.vertices[firstVertex + i + 1];
		worldVertex_t &v3 = s_worldRenderData.vertices[firstVertex + i + 2];

		// Take this opportunity to sort out the normals too
		R_BuildVertexNormals( v1, v2, v3 );
#endif

		s_worldRenderData.indices.push_back( firstVertex + 0 );
		s_worldRenderData.indices.push_back( firstVertex + i + 1 );
		s_worldRenderData.indices.push_back( firstVertex + i + 2 );
	}

	// index of the last index!
	const uint32 lastIndex = static_cast<uint32>( s_worldRenderData.indices.size() );
	const uint32 numIndices = lastIndex - firstIndex;

	fa->firstIndex = firstIndex;
	fa->numIndices = numIndices;

	assert( !( fa->numIndices % 3 ) );

#ifndef USE_PVS
	//
	// Figure out where we're creating the mesh
	//
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
#endif
}

void R_BuildWorldLists( model_t *world )
{
	//
	// Reserve at least enough space to hold all the base map vertices
	// The reserve amount is a rough approximation of what a map will need,
	// not what it will actually use
	//
	s_worldRenderData.vertices.reserve( world->numvertexes );
	s_worldRenderData.indices.reserve( world->numvertexes * 3 );

	mmodel_t *firstModel = world->submodels;
	mmodel_t *endModel = firstModel + world->numsubmodels; // Not a valid submodel

	for ( mmodel_t *subModel = firstModel; subModel < endModel; ++subModel )
	{
		const uint32 firstWorldMesh = static_cast<uint32>( s_worldRenderData.meshes.size() );
		subModel->firstMesh = firstWorldMesh;

		msurface_t *firstFace = world->surfaces + subModel->firstface;
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
			R_BuildPolygonFromSurface( surface, subModel );
		}

		const uint32 lastWorldMesh = static_cast<uint32>( s_worldRenderData.meshes.size() );
		subModel->numMeshes = lastWorldMesh - firstWorldMesh;
	}

	//
	// Upload our big fat vertex buffer
	//

	glGenVertexArrays( 1, &s_worldRenderData.vao );
	glGenBuffers( 2, &s_worldRenderData.vbo );			// Gen the vbo and ebo in one go

	glBindVertexArray( s_worldRenderData.vao );
	glBindBuffer( GL_ARRAY_BUFFER, s_worldRenderData.vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, s_worldRenderData.eboCluster );

	// xyz, st1, st2 normal
	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );
	glEnableVertexAttribArray( 2 );
	glEnableVertexAttribArray( 3 );

	constexpr GLsizei structureSize = sizeof( worldVertex_t );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, structureSize, (void *)( 0 ) );
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, structureSize, (void *)( 3 * sizeof( GLfloat ) ) );
	glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, structureSize, (void *)( 5 * sizeof( GLfloat ) ) );
	glVertexAttribPointer( 3, 3, GL_FLOAT, GL_FALSE, structureSize, (void *)( 7 * sizeof( GLfloat ) ) );

	worldVertex_t *vertexData = s_worldRenderData.vertices.data();
	const size_t vertexCount = s_worldRenderData.vertices.size();
	const size_t vertexSize = vertexCount * sizeof( worldVertex_t );

#ifndef USE_PVS
	worldIndex_t *indexData = s_worldRenderData.indices.data();
	const size_t indexCount = s_worldRenderData.indices.size();
	const size_t indexSize = indexCount * sizeof( worldIndex_t );
#endif

#ifdef USE_MESHOPT

#ifndef USE_PVS

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

#endif

	size_t numVertices = meshopt_optimizeVertexFetch(
		vertexData, indexData, indexCount,
		vertexData, vertexCount, sizeof( worldVertex_t ) );

#endif

	glBufferData( GL_ARRAY_BUFFER, vertexSize, vertexData, GL_STATIC_DRAW );
#ifndef USE_PVS
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexSize, indexData, GL_STATIC_DRAW );
#endif
}
