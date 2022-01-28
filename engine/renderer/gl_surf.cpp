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
#include <vector>

#define MAX_SHADERLIGHTS 4

#define DYNAMIC_LIGHT_WIDTH		128
#define DYNAMIC_LIGHT_HEIGHT	128

#define LIGHTMAP_BYTES	4

#define	BLOCK_WIDTH		128
#define	BLOCK_HEIGHT	128

#define	MAX_LIGHTMAPS	128

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

// This is the data sent to OpenGL
// We use a single, large vertex / index buffer
// for all world geometry
// Surfaces store indices into the index buffer
struct worldRenderData_t
{
	std::vector<worldVertex_t>	vertices;
	std::vector<worldIndex_t>	indices;

	GLuint vao, vbo, ebo;
	GLuint eboCluster;		// ebo for the current viscluster
};

// A surface batch
struct worldMesh_t
{
	msurface_t *	surface;
	uint			firstIndex;
	uint			numIndices;
};

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
static worldRenderData_t s_worldRenderData;

static vec3_t		modelorg;		// relative to viewpoint

static msurface_t	*r_alpha_surfaces;

int		c_visible_lightmaps;
int		c_visible_textures;

struct gllightmapstate_t
{
	int	current_lightmap_texture;

	msurface_t	*lightmap_surfaces[MAX_LIGHTMAPS];

	int			allocated[BLOCK_WIDTH];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte		lightmap_buffer[4*BLOCK_WIDTH*BLOCK_HEIGHT];
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

static void DrawGLPoly( glpoly_t *p )
{
	float *v = p->verts[0];

	glBegin( GL_TRIANGLES );
	for ( int i = 0; i < p->numverts; ++i, v += VERTEXSIZE )
	{
		glTexCoord2f( v[3], v[4] );
		glVertex3fv( v );
	}
	glEnd();
}

//
// Draws a wireframe view of the world
//
void R_DrawTriangleOutlines()
{
	glpoly_t *p;

	if ( !r_showtris->GetBool() ) {
		return;
	}

	glDisable( GL_TEXTURE_2D );
	glDisable( GL_DEPTH_TEST );
	glColor4f( 1, 1, 1, 1 );

	for ( int i = 1; i < MAX_LIGHTMAPS; i++ )
	{
		msurface_t *surf;

		for ( surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain )
		{
			p = surf->polys;
			for ( ; p; p = p->chain )
			{
				for ( int j = 2; j < p->numverts; j++ )
				{
					glBegin( GL_LINE_STRIP );
					glVertex3fv( p->verts[0] );
					glVertex3fv( p->verts[j - 1] );
					glVertex3fv( p->verts[j] );
					glVertex3fv( p->verts[0] );
					glEnd();
				}
			}
		}
	}

	glEnable( GL_DEPTH_TEST );
	glEnable( GL_TEXTURE_2D );
}

//
// Draw water surfaces and windows.
// The BSP tree is waled front to back, so unwinding the chain
// of alpha_surfaces will draw back to front, giving proper ordering.
//
void R_DrawAlphaSurfaces()
{
	msurface_t *s;

	glEnable( GL_BLEND );
	GL_TexEnv( GL_MODULATE );

	for ( s = r_alpha_surfaces; s; s = s->texturechain )
	{
		s->texinfo->material->Bind();
		c_brush_polys++;
		if ( s->texinfo->flags & SURF_TRANS33 )
			glColor4f( 1.0f, 1.0f, 1.0f, 0.33f );
		else if ( s->texinfo->flags & SURF_TRANS66 )
			glColor4f( 1.0f, 1.0f, 1.0f, 0.66f );
		else
			glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		if ( s->flags & SURF_DRAWTURB )
			EmitWaterPolys( s );
		else
			DrawGLPoly( s->polys );
	}

	GL_TexEnv( GL_REPLACE );
	glColor4f( 1, 1, 1, 1 );
	glDisable( GL_BLEND );

	r_alpha_surfaces = NULL;
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
#if 0
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

// When recursing the BSP tree we give each material its own array of indices, then
// when we finish, we append them all into one large array which we send to the GPU
struct worldMaterialSet_t
{
	msurface_t *surface;
	std::vector<worldIndex_t> indices;
};

struct worldNodeWork_t
{
	std::vector<worldMaterialSet_t> materialSets;
};

static void R_AddSurfaceToMaterialSet( msurface_t *surf, worldMaterialSet_t &materialSet )
{
	for ( uint i = 0; i < surf->numVertices; ++i )
	{
		materialSet.indices.push_back( surf->firstVertex + i );
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
	msurface_t *	surf, **mark;
	mleaf_t *		pleaf;
	float			dot;

	if ( node->contents == CONTENTS_SOLID ) {
		// solid
		return;
	}

	if ( node->visframe != r_visframecount ) {
		return;
	}

	if ( R_CullBox( node->minmaxs, node->minmaxs + 3 ) ) {
		return;
	}

	// if a leaf node, draw stuff
	if ( node->contents != -1 )
	{
		pleaf = (mleaf_t *)node;

		// check for door connected areas
		if ( tr.refdef.areabits )
		{
			if ( !( tr.refdef.areabits[pleaf->area >> 3] & ( 1 << ( pleaf->area & 7 ) ) ) ) {
				// not visible
				return;
			}
		}

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		while ( c-- )
		{
			( *mark )->visframe = r_framecount;
			mark++;
		}

		return;
	}

	// node is just a decision point, so go down the apropriate sides

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

	// recurse down the children, front side first
	R_RecursiveWorldNode( work, node->children[side] );

	// draw stuff
	for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c; c--, surf++ )
	{
		if ( surf->visframe != r_framecount ) {
			continue;
		}

		if ( ( surf->flags & SURF_PLANEBACK ) != sidebit ) {
			// wrong side
			continue;
		}

		if ( surf->texinfo->flags & SURF_SKY )
		{
			// just adds to visible sky bounds
			R_AddSkySurface( surf );
		}
		else if ( surf->texinfo->flags & ( SURF_TRANS33 | SURF_TRANS66 ) )
		{
		}
		else
		{
			bool added = false;
			for ( worldMaterialSet_t &materialSet : work.materialSets )
			{
				if ( materialSet.surface->texinfo == surf->texinfo )
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
				materialSet.surface = surf;
				R_AddSurfaceToMaterialSet( surf, materialSet );
			}
		}
	}

	// recurse down the back side
	R_RecursiveWorldNode( work, node->children[!side] );
}

//
// Squashes our index lists into the final index buffer, builds the world lists
//
static void R_SquashAndUploadIndices( const worldNodeWork_t &work )
{
#if 0
	for ( const worldMaterialSet_t &materialSet : work.materialSets )
	{
		// Create a new mesh
		worldMesh_t &opaqueMesh = s_worldLists.opaqueMeshes.emplace_back();
		opaqueMesh.surface = materialSet.surface;

		uint firstIndex = static_cast<uint>( s_worldLists.finalIndices.size() );

		// Insert our individual index buffer at the end of the final index buffer
		s_worldLists.finalIndices.insert( s_worldLists.finalIndices.end(), materialSet.indices.begin(), materialSet.indices.end() );

		uint endIndex = static_cast<uint>( s_worldLists.finalIndices.size() ) - 1;

		opaqueMesh.firstIndex = firstIndex;
		opaqueMesh.numIndices = endIndex;
	}

	// Squashed! Now upload the index buffer
	glBindVertexArray( s_worldRenderData.vao );
	glBindBuffer( GL_ARRAY_BUFFER, s_worldRenderData.vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, s_worldRenderData.ebo );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, s_worldLists.finalIndices.size() * sizeof( worldIndex_t ), s_worldLists.finalIndices.data(), GL_DYNAMIC_DRAW );
#endif
}

//
// Draws all opaque surfaces in the world list
//
static void R_DrawOpaqueSurfaces()
{
	glBindVertexArray( s_worldRenderData.vao );
	glBindBuffer( GL_ARRAY_BUFFER, s_worldRenderData.vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, s_worldRenderData.ebo );

#if 0
	for ( const worldMesh_t &mesh : s_worldLists.opaqueMeshes )
	{
		// Increment our counter
		//++c_brush_polys;

		const material_t *mat = R_TextureAnimation( mesh.surface->texinfo );
		const image_t *image = mat->image;

		glActiveTexture( GL_TEXTURE0 );
		mat->Bind();
		glActiveTexture( GL_TEXTURE1 );
		GL_Bind( glState.lightmap_textures + mesh.surface->lightmaptexturenum );

		glDrawElements( GL_POLYGON, mesh.numIndices, GL_UNSIGNED_INT, (void *)( (uintptr_t)mesh.firstIndex ) );
	}
#else
	glActiveTexture( GL_TEXTURE0 );
	defaultMaterial->Bind();
	glActiveTexture( GL_TEXTURE1 );
	whiteMaterial->Bind();
	//GL_Bind( glState.lightmap_textures + 1 );
//glDrawArrays( GL_POLYGON, 0, 32 );
	glDrawElements( GL_TRIANGLES, s_worldRenderData.indices.size(), GL_UNSIGNED_INT, (void *)( (uintptr_t)0 ) );
#endif
}

bool g_skippedMarkLeaves;

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

	glState.currenttextures[0] = glState.currenttextures[1] = -1;

	glColor3f( 1, 1, 1 );
	memset( gl_lms.lightmap_surfaces, 0, sizeof( gl_lms.lightmap_surfaces ) );
	R_ClearSkyBox();

	// Build the world lists
	if ( !g_skippedMarkLeaves )
	{
		worldNodeWork_t work;

		s_worldLists.ClearAllLists();

		// This figures out what we need to render and builds the world lists
		R_RecursiveWorldNode( work, r_worldmodel->nodes );

		R_SquashAndUploadIndices( work );
	}

	// SHADERWORLD
#if 1

	DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixIdentity();

	DirectX::XMFLOAT4X4A modelMatrixStore;
	DirectX::XMStoreFloat4x4A( &modelMatrixStore, modelMatrix );

	glUseProgram( glProgs.worldProg );

	glUniformMatrix4fv( 4, 1, GL_FALSE, (const GLfloat *)&modelMatrixStore );
	glUniformMatrix4fv( 5, 1, GL_FALSE, (const GLfloat *)&tr.viewMatrix );
	glUniformMatrix4fv( 6, 1, GL_FALSE, (const GLfloat *)&tr.projMatrix );

	glUniform3fv( 7, 1, tr.refdef.vieworg );

	constexpr int startIndex = 8;
	constexpr int elementsInRenderLight = 3;

	for ( int iter1 = 0, iter2 = 0; iter1 < MAX_SHADERLIGHTS; ++iter1, iter2 += elementsInRenderLight )
	{
		glUniform3fv( startIndex + iter2 + 0, 1, tr.refdef.dlights[iter1].origin );
		glUniform3fv( startIndex + iter2 + 1, 1, tr.refdef.dlights[iter1].color );
		glUniform1f( startIndex + iter2 + 2, tr.refdef.dlights[iter1].intensity );
	}

	constexpr int indexAfterLights = startIndex + elementsInRenderLight * MAX_SHADERLIGHTS;

	glUniform1i( indexAfterLights + 0, 0 ); // diffuse
	glUniform1i( indexAfterLights + 1, 1 ); // lightmap

#endif
	// SHADERWORLD

	// Now render stuff!
	R_DrawOpaqueSurfaces();

	// SHADERWORLD
#if 1

	glUseProgram( 0 );

#endif
	// SHADERWORLD

	// CLEAN UP STUFF!!!!!!!!!!!!!!!!!!!!!

	R_DrawSkyBox();

	//R_DrawTriangleOutlines();
}

// Mark the leaves and nodes that are in the PVS for the current cluster
void R_MarkLeaves()
{
	byte *		vis;
	byte		fatvis[MAX_MAP_LEAFS / 8];
	mnode_t *	node;
	int			i, c;
	mleaf_t *	leaf;
	int			cluster;

	if ( r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->GetBool() && r_viewcluster != -1 )
	{
		g_skippedMarkLeaves = true;
		return;
	}

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if ( r_lockpvs->GetBool() )
	{
		g_skippedMarkLeaves = true;
		return;
	}

	g_skippedMarkLeaves = false;

	++r_visframecount;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if ( r_novis->GetBool() || r_viewcluster == -1 || !r_worldmodel->vis )
	{
		// mark everything
		for ( i = 0; i < r_worldmodel->numleafs; i++ )
		{
			r_worldmodel->leafs[i].visframe = r_visframecount;
		}
		for ( i = 0; i < r_worldmodel->numnodes; i++ )
		{
			r_worldmodel->nodes[i].visframe = r_visframecount;
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

	for ( i = 0, leaf = r_worldmodel->leafs; i < r_worldmodel->numleafs; i++, leaf++ )
	{
		cluster = leaf->cluster;
		if ( cluster == -1 )
		{
			continue;
		}
		if ( vis[cluster >> 3] & ( 1 << ( cluster & 7 ) ) )
		{
			node = (mnode_t *)leaf;
			do
			{
				if ( node->visframe == r_visframecount )
				{
					break;
				}
				node->visframe = r_visframecount;
				node = node->parent;
			} while ( node );
		}
	}

#if 0
	for ( i = 0; i < r_worldmodel->vis->numclusters; i++ )
	{
		if ( vis[i >> 3] & ( 1 << ( i & 7 ) ) )
		{
			node = (mnode_t *)&r_worldmodel->leafs[i];	// FIXME: cluster
			do
			{
				if ( node->visframe == r_visframecount )
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while ( node );
		}
	}
#endif
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

static void LM_UploadBlock()
{
	int texture = gl_lms.current_lightmap_texture;

	GL_Bind( glState.lightmap_textures + texture );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

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
// Given an msurface_t with basic brush geometry specified
// will create optimised render geometry and append it to s_worldRenderData
// It then gives the index of the first renderindex to the msurface_t
//
void GL_BuildPolygonFromSurface( msurface_t *fa )
{
	const medge_t *edges = currentmodel->edges;
	const int numEdges = fa->numedges;

	s_worldRenderData.vertices.reserve( s_worldRenderData.vertices.size() + numEdges );

	// Index of the first vertex
	const uint firstVertex = static_cast<uint>( s_worldRenderData.vertices.size() );

	// Reconstruct the polygon

	// First, build the vertices for this face

	for ( int i = 0; i < numEdges; ++i )
	{
		const int surfEdge = currentmodel->surfedges[fa->firstedge + i];
		const int startIndex = surfEdge > 0 ? 0 : 1;

		const medge_t *edge = edges + abs( surfEdge );
		const float *vertex = currentmodel->vertexes[edge->v[startIndex]].position;

		worldVertex_t &outVertex = s_worldRenderData.vertices.emplace_back();

		// ST/UV coordinates

		float s, t;

		s = DotProduct( vertex, fa->texinfo->vecs[0] ) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->material->image->width;

		t = DotProduct( vertex, fa->texinfo->vecs[1] ) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->material->image->height;

		// Copy in the vertex positions and ST coordinates

		VectorCopy( vertex, outVertex.pos );

		outVertex.st1[0] = s;
		outVertex.st1[1] = t;

		// Lightmap ST/UV coordinates

		s = DotProduct( vertex, fa->texinfo->vecs[0] ) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16; // fa->texinfo->texture->width;

		t = DotProduct( vertex, fa->texinfo->vecs[1] ) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16; // fa->texinfo->texture->height;

		outVertex.st2[0] = s;
		outVertex.st2[1] = t;
	}

	const int numTriangles = fa->numedges - 2;

	if ( fa->numedges == 3 )
	{
		// Already triangulated!
		s_worldRenderData.indices.push_back( firstVertex + 0 );
		s_worldRenderData.indices.push_back( firstVertex + 1 );
		s_worldRenderData.indices.push_back( firstVertex + 2 );
		return;
	}

	// triangulate it, fan style

	int i;
	int middle;

	// for every triangle
	for ( i = 0, middle = 1; i < numTriangles; ++i, ++middle )
	{
		s_worldRenderData.indices.push_back( firstVertex + 0 );
		s_worldRenderData.indices.push_back( firstVertex + middle );
		s_worldRenderData.indices.push_back( firstVertex + middle + 1 );
	}

	// Index of the last vertex
	const uint endVertex = static_cast<uint>( s_worldRenderData.vertices.size() );

	fa->firstVertex = firstVertex;
	fa->numVertices = endVertex - firstVertex;

	assert( fa->numVertices == numEdges );
}

//
// This should only ever be called between
// GL_BeginBuildingLightmaps and
// GL_EndBuildingLightmaps
//
void GL_CreateSurfaceLightmap( msurface_t *surf )
{
	if ( surf->flags & ( SURF_DRAWSKY | SURF_DRAWTURB ) )
	{
		return;
	}

	int smax = ( surf->extents[0] >> 4 ) + 1;
	int tmax = ( surf->extents[1] >> 4 ) + 1;

	if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
	{
		LM_UploadBlock();
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

void GL_BeginBuildingLightmaps( model_t *m )
{
	static lightstyle_t	lightstyles[MAX_LIGHTSTYLES];

	memset( gl_lms.allocated, 0, sizeof( gl_lms.allocated ) );

	r_framecount = 1; // no dlightcache

	GL_EnableMultitexture( true );
	GL_SelectTexture( GL_TEXTURE1 );

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
	GL_Bind( glState.lightmap_textures + 0 );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D,
		0,
		GL_RGBA8,
		BLOCK_WIDTH, BLOCK_HEIGHT,
		0,
		GL_LIGHTMAP_FORMAT,
		GL_UNSIGNED_BYTE,
		nullptr );

	//
	// Create the mesh for the world
	//
}

void GL_EndBuildingLightmaps()
{
	LM_UploadBlock();
	GL_EnableMultitexture( false );

	// Upload our big fat vertex buffer

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

	constexpr GLsizei structureSize = sizeof( worldVertex_t );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, structureSize, (void *)( 0 ) );
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, structureSize, (void *)( 3 * sizeof( GLfloat ) ) );
	glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, structureSize, (void *)( 5 * sizeof( GLfloat ) ) );
	glVertexAttribPointer( 3, 3, GL_FLOAT, GL_FALSE, structureSize, (void *)( 7 * sizeof( GLfloat ) ) );

	const void *vertexData = reinterpret_cast<const void *>( s_worldRenderData.vertices.data() );
	const GLsizeiptr vertexSize = static_cast<GLsizeiptr>( s_worldRenderData.vertices.size() ) * sizeof( worldVertex_t );

	const void *indexData = reinterpret_cast<const void *>( s_worldRenderData.indices.data() );
	const GLsizeiptr indexSize = static_cast<GLsizeiptr>( s_worldRenderData.indices.size() ) * sizeof( worldIndex_t );

	glBufferData( GL_ARRAY_BUFFER, vertexSize, vertexData, GL_STATIC_DRAW );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexSize, indexData, GL_STATIC_DRAW );
}
