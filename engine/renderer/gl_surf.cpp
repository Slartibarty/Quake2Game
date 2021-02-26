//-------------------------------------------------------------------------------------------------
// Surface-related refresh code
//-------------------------------------------------------------------------------------------------

#include "gl_local.h"

#include "../shared/imageloaders.h"
#include <vector>

#include "xatlas.h"

#define DYNAMIC_LIGHT_WIDTH		128
#define DYNAMIC_LIGHT_HEIGHT	128

#define LIGHTMAP_BYTES	4

#define	BLOCK_WIDTH		128
#define	BLOCK_HEIGHT	128

#define	MAX_LIGHTMAPS	128

#define GL_LIGHTMAP_FORMAT GL_RGBA

#define TEXNUM_LIGHTMAPS	1024

// Globals

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

// Prototypes

static void		LM_InitBlock( void );
static void		LM_UploadBlock( qboolean dynamic );
static qboolean	LM_AllocBlock (int w, int h, int *x, int *y);

// gl_light
void R_SetCacheState(msurface_t *surf);
void R_BuildLightMap(msurface_t *surf, byte *dest, int stride);

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
static material_t *R_TextureAnimation (mtexinfo_t *tex)
{
	int c;

	if (!tex->next)
		return tex->material;

	c = currententity->frame % tex->numframes;
	while (c)
	{
		tex = tex->next;
		c--;
	}

	return tex->material;
}

/*
================
DrawGLPoly
================
*/
static void DrawGLPoly (glpoly_t *p)
{
	int		i;
	float	*v;

	glBegin (GL_POLYGON);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
	{
		glTexCoord2f (v[3], v[4]);
		glVertex3fv (v);
	}
	glEnd ();
}

//============
//PGM
/*
================
DrawGLFlowingPoly -- version of DrawGLPoly that handles scrolling texture
================
*/
static void DrawGLFlowingPoly (msurface_t *fa)
{
	int		i;
	float	*v;
	glpoly_t *p;
	float	scroll;

	p = fa->polys;

	scroll = -64.0f * ( (r_newrefdef.time / 40.0f) - (int)(r_newrefdef.time / 40.0f) );
	if(scroll == 0.0f)
		scroll = -64.0f;

	glBegin (GL_POLYGON);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
	{
		glTexCoord2f ((v[3] + scroll), v[4]);
		glVertex3fv (v);
	}
	glEnd ();
}
//PGM
//============

/*
** R_DrawTriangleOutlines
*/
void R_DrawTriangleOutlines (void)
{
	int			i, j;
	glpoly_t	*p;

	if (!gl_showtris->value)
		return;

	glDisable (GL_TEXTURE_2D);
	glDisable (GL_DEPTH_TEST);
	glColor4f (1,1,1,1);

	for (i=1 ; i<MAX_LIGHTMAPS ; i++)
	{
		msurface_t *surf;

		for ( surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain )
		{
			p = surf->polys;
			for ( ; p ; p=p->chain)
			{
				for (j=2 ; j<p->numverts ; j++ )
				{
					glBegin (GL_LINE_STRIP);
					glVertex3fv (p->verts[0]);
					glVertex3fv (p->verts[j-1]);
					glVertex3fv (p->verts[j]);
					glVertex3fv (p->verts[0]);
					glEnd ();
				}
			}
		}
	}

	glEnable (GL_DEPTH_TEST);
	glEnable (GL_TEXTURE_2D);
}

/*
** DrawGLPolyChain
*/
void DrawGLPolyChain( glpoly_t *p, float soffset, float toffset )
{
	float *v;
	int j;

	if ( soffset == 0 && toffset == 0 )
	{
		for ( ; p != 0; p = p->chain )
		{
			glBegin (GL_POLYGON);
			v = p->verts[0];
			for (j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE)
			{
				glTexCoord2f (v[5], v[6] );
				glVertex3fv (v);
			}
			glEnd ();
		}
	}
	else
	{
		for ( ; p != 0; p = p->chain )
		{
			glBegin (GL_POLYGON);
			v = p->verts[0];
			for (j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE)
			{
				glTexCoord2f (v[5] - soffset, v[6] - toffset );
				glVertex3fv (v);
			}
			glEnd ();
		}
	}
}

/*
** R_BlendLightMaps
**
** This routine takes all the given light mapped surfaces in the world and
** blends them into the framebuffer.
*/
void R_BlendLightmaps (void)
{
	int			i;
	msurface_t	*surf, *newdrawsurf = NULL;

	// don't bother if we're set to fullbright
	if (r_fullbright->value)
		return;
	if (!r_worldmodel->lightdata)
		return;

	// don't bother writing Z
	glDepthMask( 0 );

	/*
	** set the appropriate blending mode unless we're only looking at the
	** lightmaps.
	*/
	if (!gl_lightmap->value)
	{
		glEnable(GL_BLEND);

		if (gl_overbright->value)
		{
			glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
		}
		else
		{
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		}
	}

	if ( currentmodel == r_worldmodel )
		c_visible_lightmaps = 0;

	/*
	** render static lightmaps first
	*/
	for ( i = 1; i < MAX_LIGHTMAPS; i++ )
	{
		if ( gl_lms.lightmap_surfaces[i] )
		{
			if (currentmodel == r_worldmodel)
				c_visible_lightmaps++;
			GL_Bind( gl_state.lightmap_textures + i);

			for ( surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain )
			{
				if ( surf->polys )
					DrawGLPolyChain( surf->polys, 0, 0 );
			}
		}
	}

	/*
	** render dynamic lightmaps
	*/
	if ( gl_dynamic->value )
	{
		LM_InitBlock();

		GL_Bind( gl_state.lightmap_textures+0 );

		if (currentmodel == r_worldmodel)
			c_visible_lightmaps++;

		newdrawsurf = gl_lms.lightmap_surfaces[0];

		for ( surf = gl_lms.lightmap_surfaces[0]; surf != 0; surf = surf->lightmapchain )
		{
			int		smax, tmax;
			byte	*base;

			smax = (surf->extents[0]>>4)+1;
			tmax = (surf->extents[1]>>4)+1;

			if ( LM_AllocBlock( smax, tmax, &surf->dlight_s, &surf->dlight_t ) )
			{
				base = gl_lms.lightmap_buffer;
				base += ( surf->dlight_t * BLOCK_WIDTH + surf->dlight_s ) * LIGHTMAP_BYTES;

				R_BuildLightMap (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
			}
			else
			{
				msurface_t *drawsurf;

				// upload what we have so far
				LM_UploadBlock( true );

				// draw all surfaces that use this lightmap
				for ( drawsurf = newdrawsurf; drawsurf != surf; drawsurf = drawsurf->lightmapchain )
				{
					if ( drawsurf->polys )
						DrawGLPolyChain( drawsurf->polys, 
										  ( drawsurf->light_s - drawsurf->dlight_s ) * ( 1.0f / 128.0f ), 
										( drawsurf->light_t - drawsurf->dlight_t ) * ( 1.0f / 128.0f ) );
				}

				newdrawsurf = drawsurf;

				// clear the block
				LM_InitBlock();

				// try uploading the block now
				if ( !LM_AllocBlock( smax, tmax, &surf->dlight_s, &surf->dlight_t ) )
				{
					Com_Error( ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed (dynamic)\n", smax, tmax );
				}

				base = gl_lms.lightmap_buffer;
				base += ( surf->dlight_t * BLOCK_WIDTH + surf->dlight_s ) * LIGHTMAP_BYTES;

				R_BuildLightMap (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
			}
		}

		/*
		** draw remainder of dynamic lightmaps that haven't been uploaded yet
		*/
		if ( newdrawsurf )
			LM_UploadBlock( true );

		for ( surf = newdrawsurf; surf != 0; surf = surf->lightmapchain )
		{
			if ( surf->polys )
				DrawGLPolyChain( surf->polys, ( surf->light_s - surf->dlight_s ) * ( 1.0f / 128.0f ), ( surf->light_t - surf->dlight_t ) * ( 1.0f / 128.0f ) );
		}
	}

	/*
	** restore state
	*/
	glDisable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask( 1 );
}

/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly (msurface_t *fa)
{
	int			maps;
	material_t	*mat;
	qboolean	is_dynamic = false;

	c_brush_polys++;

	mat = R_TextureAnimation (fa->texinfo);

	if (fa->flags & SURF_DRAWTURB)
	{
		mat->Bind();

		// warp texture, no lightmaps
		GL_TexEnv( GL_MODULATE );
		// SLARTTODO
		//glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		EmitWaterPolys (fa);
		GL_TexEnv( GL_REPLACE );

		return;
	}
	else
	{
		mat->Bind();

		GL_TexEnv( GL_REPLACE );
	}

//======
//PGM
	if(fa->texinfo->flags & SURF_FLOWING)
		DrawGLFlowingPoly (fa);
	else
		DrawGLPoly (fa->polys);
//PGM
//======

	/*
	** check for lightmap modification
	*/
	for ( maps = 0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++ )
	{
		if ( r_newrefdef.lightstyles[fa->styles[maps]].white != fa->cached_light[maps] )
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if ( fa->dlightframe == r_framecount )
	{
dynamic:
		if ( gl_dynamic->value )
		{
			if (!( fa->texinfo->flags & (SURFMASK_UNLIT ) ) )
			{
				is_dynamic = true;
			}
		}
	}

	if ( is_dynamic )
	{
		if ( ( fa->styles[maps] >= 32 || fa->styles[maps] == 0 ) && ( fa->dlightframe != r_framecount ) )
		{
			unsigned	temp[34*34];
			int			smax, tmax;

			smax = (fa->extents[0]>>4)+1;
			tmax = (fa->extents[1]>>4)+1;

			R_BuildLightMap( fa, (byte *)temp, smax*4 );
			R_SetCacheState( fa );

			GL_Bind( gl_state.lightmap_textures + fa->lightmaptexturenum );

			glTexSubImage2D( GL_TEXTURE_2D, 0,
							  fa->light_s, fa->light_t, 
							  smax, tmax, 
							  GL_LIGHTMAP_FORMAT, 
							  GL_UNSIGNED_BYTE, temp );

			fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
			gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
		}
		else
		{
			fa->lightmapchain = gl_lms.lightmap_surfaces[0];
			gl_lms.lightmap_surfaces[0] = fa;
		}
	}
	else
	{
		fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
		gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
	}
}


/*
================
R_DrawAlphaSurfaces

Draw water surfaces and windows.
The BSP tree is waled front to back, so unwinding the chain
of alpha_surfaces will draw back to front, giving proper ordering.
================
*/
void R_DrawAlphaSurfaces (void)
{
	msurface_t	*s;

	//
	// go back to the world matrix
	//
	glLoadMatrixf (r_world_matrix);

	glEnable (GL_BLEND);
	GL_TexEnv( GL_MODULATE );

	for (s=r_alpha_surfaces ; s ; s=s->texturechain)
	{
		s->texinfo->material->Bind();
		c_brush_polys++;
		if (s->texinfo->flags & SURF_TRANS33)
			glColor4f (1.0f,1.0f,1.0f,0.33f);
		else if (s->texinfo->flags & SURF_TRANS66)
			glColor4f (1.0f,1.0f,1.0f,0.66f);
		else
			glColor4f (1.0f,1.0f,1.0f,1.0f);
		if (s->flags & SURF_DRAWTURB)
			EmitWaterPolys (s);
		else if(s->texinfo->flags & SURF_FLOWING)			// PGM	9/16/98
			DrawGLFlowingPoly (s);							// PGM
		else
			DrawGLPoly (s->polys);
	}

	GL_TexEnv( GL_REPLACE );
	glColor4f (1,1,1,1);
	glDisable (GL_BLEND);

	r_alpha_surfaces = NULL;
}

/*
================
DrawTextureChains
================
*/
void DrawTextureChains (void)
{
	int		i;
	msurface_t	*s;
	material_t	*mat;

	c_visible_textures = 0;

//	GL_TexEnv( GL_REPLACE );

	if ( !GLEW_ARB_multitexture || !gl_ext_multitexture->value )
	{
		for ( i = 0, mat=glmaterials ; i<numglmaterials ; i++,mat++)
		{
			if (!mat->registration_sequence)
				continue;
			s = mat->texturechain;
			if (!s)
				continue;
			c_visible_textures++;

			for ( ; s ; s=s->texturechain)
				R_RenderBrushPoly (s);

			mat->texturechain = NULL;
		}
	}
	else
	{
		for ( i = 0, mat=glmaterials ; i<numglmaterials ; i++,mat++)
		{
			if (!mat->registration_sequence)
				continue;
			if (!mat->texturechain)
				continue;
			c_visible_textures++;

			for ( s = mat->texturechain; s ; s=s->texturechain)
			{
				if ( !( s->flags & SURF_DRAWTURB ) )
					R_RenderBrushPoly (s);
			}
		}

		GL_EnableMultitexture( false );
		for ( i = 0, mat=glmaterials ; i<numglmaterials ; i++,mat++)
		{
			if (!mat->registration_sequence)
				continue;
			s = mat->texturechain;
			if (!s)
				continue;

			for ( ; s ; s=s->texturechain)
			{
				if ( s->flags & SURF_DRAWTURB )
					R_RenderBrushPoly (s);
			}

			mat->texturechain = NULL;
		}
//		GL_EnableMultitexture( true );
	}

	GL_TexEnv( GL_REPLACE );
}


static void GL_RenderLightmappedPoly( msurface_t *surf )
{
	int		i, nv = surf->polys->numverts;
	int		map;
	float	*v;
	material_t *mat = R_TextureAnimation( surf->texinfo );
	image_t *image = mat->image;
	bool is_dynamic = false;
	unsigned lmtex = surf->lightmaptexturenum;
	glpoly_t *p;

	for ( map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255; map++ )
	{
		if ( r_newrefdef.lightstyles[surf->styles[map]].white != surf->cached_light[map] )
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if ( surf->dlightframe == r_framecount )
	{
dynamic:
		if ( gl_dynamic->value )
		{
			if ( !(surf->texinfo->flags & (SURFMASK_UNLIT ) ) )
			{
				is_dynamic = true;
			}
		}
	}

	if ( is_dynamic )
	{
		unsigned	temp[128*128]; // SlartTodo: WTF??????????
		int			smax, tmax;

		if ( ( surf->styles[map] >= 32 || surf->styles[map] == 0 ) && ( surf->dlightframe != r_framecount ) )
		{
			smax = (surf->extents[0]>>4)+1;
			tmax = (surf->extents[1]>>4)+1;

			R_BuildLightMap( surf, (byte *)temp, smax*4 );
			R_SetCacheState( surf );

			GL_MBind( GL_TEXTURE1, gl_state.lightmap_textures + surf->lightmaptexturenum );

			lmtex = surf->lightmaptexturenum;

			glTexSubImage2D( GL_TEXTURE_2D, 0,
							  surf->light_s, surf->light_t, 
							  smax, tmax, 
							  GL_LIGHTMAP_FORMAT, 
							  GL_UNSIGNED_BYTE, temp );

		}
		else
		{
			smax = (surf->extents[0]>>4)+1;
			tmax = (surf->extents[1]>>4)+1;

			R_BuildLightMap( surf, (byte *)temp, smax*4 );

			GL_MBind( GL_TEXTURE1, gl_state.lightmap_textures + 0 );

			lmtex = 0;

			glTexSubImage2D( GL_TEXTURE_2D, 0,
							  surf->light_s, surf->light_t, 
							  smax, tmax, 
							  GL_LIGHTMAP_FORMAT, 
							  GL_UNSIGNED_BYTE, temp );

		}

		c_brush_polys++;

		GL_MBind( GL_TEXTURE0, image->texnum );
		GL_MBind( GL_TEXTURE1, gl_state.lightmap_textures + lmtex );

//==========
//PGM
		if (surf->texinfo->flags & SURF_FLOWING)
		{
			float scroll;
		
			scroll = -64.0f * ( (r_newrefdef.time / 40.0f) - (int)(r_newrefdef.time / 40.0f) );
			if(scroll == 0.0f)
				scroll = -64.0f;

			for ( p = surf->polys; p; p = p->chain )
			{
				v = p->verts[0];
				glBegin (GL_POLYGON);
				for (i=0 ; i< nv; i++, v+= VERTEXSIZE)
				{
					glMultiTexCoord2fARB( GL_TEXTURE0, (v[3]+scroll), v[4]);
					glMultiTexCoord2fARB( GL_TEXTURE1, v[5], v[6]);
					glVertex3fv (v);
				}
				glEnd ();
			}
		}
		else
		{
			for ( p = surf->polys; p; p = p->chain )
			{
				v = p->verts[0];
				glBegin (GL_POLYGON);
				for (i=0 ; i< nv; i++, v+= VERTEXSIZE)
				{
					glMultiTexCoord2fARB( GL_TEXTURE0, v[3], v[4]);
					glMultiTexCoord2fARB( GL_TEXTURE1, v[5], v[6]);
					glVertex3fv (v);
				}
				glEnd ();
			}
		}
//PGM
//==========
	}
	else
	{
		c_brush_polys++;

		GL_MBind( GL_TEXTURE0, image->texnum );
		GL_MBind( GL_TEXTURE1, gl_state.lightmap_textures + lmtex );

//==========
//PGM
		if (surf->texinfo->flags & SURF_FLOWING)
		{
			float scroll;
		
			scroll = -64.f * ( (r_newrefdef.time / 40.0f) - (int)(r_newrefdef.time / 40.0f) );
			if(scroll == 0.0f)
				scroll = -64.0f;

			for ( p = surf->polys; p; p = p->chain )
			{
				v = p->verts[0];
				glBegin (GL_POLYGON);
				for (i=0 ; i< nv; i++, v+= VERTEXSIZE)
				{
					glMultiTexCoord2fARB( GL_TEXTURE0, (v[3]+scroll), v[4]);
					glMultiTexCoord2fARB( GL_TEXTURE1, v[5], v[6]);
					glVertex3fv (v);
				}
				glEnd ();
			}
		}
		else
		{
//PGM
//==========
			for ( p = surf->polys; p; p = p->chain )
			{
				v = p->verts[0];
				glBegin (GL_POLYGON);
				for (i=0 ; i< nv; i++, v+= VERTEXSIZE)
				{
					glMultiTexCoord2fARB( GL_TEXTURE0, v[3], v[4]);
					glMultiTexCoord2fARB( GL_TEXTURE1, v[5], v[6]);
					glVertex3fv (v);
				}
				glEnd ();
			}
//==========
//PGM
		}
//PGM
//==========
	}
}

/*
=================
R_DrawInlineBModel
=================
*/
void R_DrawInlineBModel (void)
{
	int			i, k;
	cplane_t	*pplane;
	float		dot;
	msurface_t	*psurf;
	dlight_t	*lt;

	// calculate dynamic lighting for bmodel
	if ( !gl_flashblend->value )
	{
		lt = r_newrefdef.dlights;
		for (k=0 ; k<r_newrefdef.num_dlights ; k++, lt++)
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
			else if ( ( GLEW_ARB_multitexture && gl_ext_multitexture->value ) && !( psurf->flags & SURF_DRAWTURB ) )
			{
				GL_RenderLightmappedPoly( psurf );
			}
			else
			{
				GL_EnableMultitexture( false );
				R_RenderBrushPoly( psurf );
				GL_EnableMultitexture( true );
			}
		}
	}

	if ( !(currententity->flags & RF_TRANSLUCENT) )
	{
		if ( !GLEW_ARB_multitexture || !gl_ext_multitexture->value )
			R_BlendLightmaps ();
	}
	else
	{
		glDisable (GL_BLEND);
		glColor4f (1,1,1,1);
		GL_TexEnv( GL_REPLACE );
	}
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (entity_t *e)
{
	vec3_t		mins, maxs;
	int			i;
	qboolean	rotated;

	if (currentmodel->nummodelsurfaces == 0)
		return;

	currententity = e;
	gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;

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

	VectorSubtract (r_newrefdef.vieworg, e->origin, modelorg);
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
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode (mnode_t *node)
{
	int			c, side, sidebit;
	cplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	float		dot;
	material_t	*mat;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;
	if (R_CullBox (node->minmaxs, node->minmaxs+3))
		return;
	
// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mleaf_t *)node;

		// check for door connected areas
		if (r_newrefdef.areabits)
		{
			if (! (r_newrefdef.areabits[pleaf->area>>3] & (1<<(pleaf->area&7)) ) )
				return;		// not visible
		}

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = r_framecount;
				mark++;
			} while (--c);
		}

		return;
	}

// node is just a decision point, so go down the apropriate sides

// find which side of the node we are on
	plane = node->plane;

	switch (plane->type)
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
		dot = DotProduct (modelorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0)
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
	R_RecursiveWorldNode (node->children[side]);

	// draw stuff
	for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		if ( (surf->flags & SURF_PLANEBACK) != sidebit )
			continue;		// wrong side

		if (surf->texinfo->flags & SURF_SKY)
		{	// just adds to visible sky bounds
			R_AddSkySurface (surf);
		}
		else if (surf->texinfo->flags & (SURF_TRANS33|SURF_TRANS66))
		{	// add to the translucent chain
			surf->texturechain = r_alpha_surfaces;
			r_alpha_surfaces = surf;
		}
		else
		{
			if ( ( GLEW_ARB_multitexture && gl_ext_multitexture->value ) && !( surf->flags & SURF_DRAWTURB ) )
			{
				GL_RenderLightmappedPoly( surf );
			}
			else
			{
				// the polygon is visible, so add it to the texture
				// sorted chain
				// FIXME: this is a hack for animation
				mat = R_TextureAnimation (surf->texinfo);
				surf->texturechain = mat->texturechain;
				mat->texturechain = surf;
			}
		}
	}

	// recurse down the back side
	R_RecursiveWorldNode (node->children[!side]);
}


/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld (void)
{
	entity_t	ent;

	if (!r_drawworld->value)
		return;

	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
		return;

	currentmodel = r_worldmodel;

	VectorCopy (r_newrefdef.vieworg, modelorg);

	// auto cycle the world frame for texture animation
	memset (&ent, 0, sizeof(ent));
	ent.frame = (int)(r_newrefdef.time*2);
	currententity = &ent;

	gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;

	glColor3f (1,1,1);
	memset (gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));
	R_ClearSkyBox ();

	if ( GLEW_ARB_multitexture && gl_ext_multitexture->value )
	{
		GL_EnableMultitexture( true );

		GL_SelectTexture( GL_TEXTURE0);
		GL_TexEnv( GL_REPLACE );
		GL_SelectTexture( GL_TEXTURE1);

		if ( gl_lightmap->value )
			GL_TexEnv( GL_REPLACE );
		else 
			GL_TexEnv( GL_MODULATE );

		R_RecursiveWorldNode (r_worldmodel->nodes);

		GL_EnableMultitexture( false );
	}
	else
	{
		R_RecursiveWorldNode (r_worldmodel->nodes);
	}

	/*
	** theoretically nothing should happen in the next two functions
	** if multitexture is enabled
	*/
	DrawTextureChains ();
	R_BlendLightmaps ();
	
	R_DrawSkyBox ();

	R_DrawTriangleOutlines ();
}


/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
void R_MarkLeaves (void)
{
	byte	*vis;
	byte	fatvis[MAX_MAP_LEAFS/8];
	mnode_t	*node;
	int		i, c;
	mleaf_t	*leaf;
	int		cluster;

	if (r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->value && r_viewcluster != -1)
		return;

	// development aid to let you run around and see exactly where
	// the pvs ends
	if (gl_lockpvs->value)
		return;

	r_visframecount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->value || r_viewcluster == -1 || !r_worldmodel->vis)
	{
		// mark everything
		for (i=0 ; i<r_worldmodel->numleafs ; i++)
			r_worldmodel->leafs[i].visframe = r_visframecount;
		for (i=0 ; i<r_worldmodel->numnodes ; i++)
			r_worldmodel->nodes[i].visframe = r_visframecount;
		return;
	}

	vis = Mod_ClusterPVS (r_viewcluster, r_worldmodel);
	// may have to combine two clusters because of solid water boundaries
	if (r_viewcluster2 != r_viewcluster)
	{
		memcpy (fatvis, vis, (r_worldmodel->numleafs+7)/8);
		vis = Mod_ClusterPVS (r_viewcluster2, r_worldmodel);
		c = (r_worldmodel->numleafs+31)/32;
		for (i=0 ; i<c ; i++)
			((int *)fatvis)[i] |= ((int *)vis)[i];
		vis = fatvis;
	}
	
	for (i=0,leaf=r_worldmodel->leafs ; i<r_worldmodel->numleafs ; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;
		if (vis[cluster>>3] & (1<<(cluster&7)))
		{
			node = (mnode_t *)leaf;
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}

#if 0
	for (i=0 ; i<r_worldmodel->vis->numclusters ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mnode_t *)&r_worldmodel->leafs[i];	// FIXME: cluster
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
#endif
}


/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

static void LM_InitBlock( void )
{
	memset( gl_lms.allocated, 0, sizeof( gl_lms.allocated ) );
}

static void LM_UploadBlock( qboolean dynamic )
{
	int texture;
	int height = 0;

	if ( dynamic )
	{
		texture = 0;
	}
	else
	{
		texture = gl_lms.current_lightmap_texture;
	}

	GL_Bind( gl_state.lightmap_textures + texture );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if ( dynamic )
	{
		int i;

		for ( i = 0; i < BLOCK_WIDTH; i++ )
		{
			if ( gl_lms.allocated[i] > height )
				height = gl_lms.allocated[i];
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
		if ( ++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS )
			Com_Error( ERR_DROP, "LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n" );
	}
}

// returns a texture number and the position inside it
static qboolean LM_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;

	best = BLOCK_HEIGHT;

	for (i=0 ; i<BLOCK_WIDTH-w ; i++)
	{
		best2 = 0;

		for (j=0 ; j<w ; j++)
		{
			if (gl_lms.allocated[i+j] >= best)
				break;
			if (gl_lms.allocated[i+j] > best2)
				best2 = gl_lms.allocated[i+j];
		}
		if (j == w)
		{	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > BLOCK_HEIGHT)
		return false;

	for (i=0 ; i<w ; i++)
		gl_lms.allocated[*x + i] = best + h;

	return true;
}

/*
================
GL_BuildPolygonFromSurface
================
*/
void GL_BuildPolygonFromSurface(msurface_t *fa)
{
	int			i, lindex, lnumverts;
	medge_t		*pedges, *r_pedge;
	int			vertpage;
	float		*vec;
	float		s, t;
	glpoly_t	*poly;
	vec3_t		total;

// reconstruct the polygon
	pedges = currentmodel->edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	VectorClear (total);
	//
	// draw texture
	//
	poly = (glpoly_t*)Hunk_Alloc (sizeof(glpoly_t) + (lnumverts-4) * VERTEXSIZE*sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i=0 ; i<lnumverts ; i++)
	{
		lindex = currentmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = currentmodel->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = currentmodel->vertexes[r_pedge->v[1]].position;
		}
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->material->image->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->material->image->height;

		VectorAdd (total, vec, total);
		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s*16;
		s += 8;
		s /= BLOCK_WIDTH*16; //fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t*16;
		t += 8;
		t /= BLOCK_HEIGHT*16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	poly->numverts = lnumverts;

}

/*
========================
GL_CreateSurfaceLightmap
========================
*/
void GL_CreateSurfaceLightmap (msurface_t *surf)
{
	int		smax, tmax;
	byte	*base;

	if (surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB))
		return;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
	{
		LM_UploadBlock( false );
		LM_InitBlock();
		if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
		{
			Com_Error( ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax );
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

	base = gl_lms.lightmap_buffer;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * LIGHTMAP_BYTES;

	R_SetCacheState( surf );
	R_BuildLightMap (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
}


/*
==================
GL_BeginBuildingLightmaps

==================
*/
void GL_BeginBuildingLightmaps (model_t *m)
{
	static lightstyle_t	lightstyles[MAX_LIGHTSTYLES];
	int				i;

	memset( gl_lms.allocated, 0, sizeof(gl_lms.allocated) );

	r_framecount = 1;		// no dlightcache

	GL_EnableMultitexture( true );
	GL_SelectTexture( GL_TEXTURE1);

	/*
	** setup the base lightstyles so the lightmaps won't have to be regenerated
	** the first time they're seen
	*/
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		lightstyles[i].rgb[0] = 1;
		lightstyles[i].rgb[1] = 1;
		lightstyles[i].rgb[2] = 1;
		lightstyles[i].white = 3;
	}
	r_newrefdef.lightstyles = lightstyles;

	if (!gl_state.lightmap_textures)
	{
		gl_state.lightmap_textures	= TEXNUM_LIGHTMAPS;
//		gl_state.lightmap_textures	= gl_state.texture_extension_number;
//		gl_state.texture_extension_number = gl_state.lightmap_textures + MAX_LIGHTMAPS;
	}

	gl_lms.current_lightmap_texture = 1;

	/*
	** initialize the dynamic lightmap texture
	*/
	GL_Bind( gl_state.lightmap_textures + 0 );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D( GL_TEXTURE_2D, 
				   0, 
				   GL_RGBA8,
				   BLOCK_WIDTH, BLOCK_HEIGHT, 
				   0, 
				   GL_LIGHTMAP_FORMAT,
				   GL_UNSIGNED_BYTE, 
				   nullptr );
}

/*
=======================
GL_EndBuildingLightmaps
=======================
*/
void GL_EndBuildingLightmaps (void)
{
	LM_UploadBlock( false );
	GL_EnableMultitexture( false );
}

/*
===============================================================================

	LIGHTMAP ATLAS GENERATION

===============================================================================
*/

#define GARBAGE

#ifdef GARBAGE

static void RandomColor(uint8_t *color)
{
	for (int i = 0; i < 3; i++)
		color[i] = uint8_t((rand() % 255 + 192) * 0.5f);
}

static void SetPixel(uint8_t *dest, int destWidth, int x, int y, const uint8_t *color)
{
	uint8_t *pixel = &dest[x * 3 + y * (destWidth * 3)];
	pixel[0] = color[0];
	pixel[1] = color[1];
	pixel[2] = color[2];
}

// https://github.com/miloyip/line/blob/master/line_bresenham.c
// License: public domain.
static void RasterizeLine(uint8_t *dest, int destWidth, const int *p1, const int *p2, const uint8_t *color)
{
	const int dx = abs(p2[0] - p1[0]), sx = p1[0] < p2[0] ? 1 : -1;
	const int dy = abs(p2[1] - p1[1]), sy = p1[1] < p2[1] ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2;
	int current[2];
	current[0] = p1[0];
	current[1] = p1[1];
	while (SetPixel(dest, destWidth, current[0], current[1], color), current[0] != p2[0] || current[1] != p2[1])
	{
		const int e2 = err;
		if (e2 > -dx) { err -= dy; current[0] += sx; }
		if (e2 < dy) { err += dx; current[1] += sy; }
	}
}

/*
https://github.com/ssloy/tinyrenderer/wiki/Lesson-2:-Triangle-rasterization-and-back-face-culling
Copyright Dmitry V. Sokolov

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
static void RasterizeTriangle(uint8_t *dest, int destWidth, const int *t0, const int *t1, const int *t2, const uint8_t *color)
{
	if (t0[1] > t1[1]) std::swap(t0, t1);
	if (t0[1] > t2[1]) std::swap(t0, t2);
	if (t1[1] > t2[1]) std::swap(t1, t2);
	int total_height = t2[1] - t0[1];
	for (int i = 0; i < total_height; i++) {
		bool second_half = i > t1[1] - t0[1] || t1[1] == t0[1];
		int segment_height = second_half ? t2[1] - t1[1] : t1[1] - t0[1];
		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? t1[1] - t0[1] : 0)) / segment_height;
		int A[2], B[2];
		for (int j = 0; j < 2; j++) {
			A[j] = int(t0[j] + (t2[j] - t0[j]) * alpha);
			B[j] = int(second_half ? t1[j] + (t2[j] - t1[j]) * beta : t0[j] + (t1[j] - t0[j]) * beta);
		}
		if (A[0] > B[0]) std::swap(A, B);
		for (int j = A[0]; j <= B[0]; j++)
			SetPixel(dest, destWidth, j, t0[1] + i, color);
	}
}

//  public-domain code by Darel Rex Finley, 2007
// http://alienryderflex.com/polygon_fill/
static void RasterizePolygon(uint8_t *dest, int destWidth, int vertices[][2], const int vertexCount, const uint8_t *color)
{
	int IMAGE_TOP = INT_MAX, IMAGE_BOT = 0, IMAGE_LEFT = INT_MAX, IMAGE_RIGHT = 0;
	for (int i = 0; i < vertexCount; i++) {
		const int *vertex = vertices[i];
		IMAGE_TOP = vertex[1] < IMAGE_TOP ? vertex[1] : IMAGE_TOP;
		IMAGE_BOT = vertex[1] > IMAGE_BOT ? vertex[1] : IMAGE_BOT;
		IMAGE_LEFT = vertex[0] < IMAGE_LEFT ? vertex[0] : IMAGE_LEFT;
		IMAGE_RIGHT = vertex[0] > IMAGE_RIGHT ? vertex[0] : IMAGE_RIGHT;
	}
	int  nodes, nodeX[255], pixelX, pixelY, i, j, swap;
	//  Loop through the rows of the image.
	for (pixelY=IMAGE_TOP; pixelY<IMAGE_BOT; pixelY++) {
		//  Build a list of nodes.
		nodes=0; j=vertexCount-1;
		for (i=0; i<vertexCount; i++) {
			if (vertices[i][1]<(double) pixelY && vertices[j][1]>=(double) pixelY || vertices[j][1]<(double) pixelY && vertices[i][1]>=(double) pixelY) {
				nodeX[nodes++]=(int) (vertices[i][0]+(pixelY-vertices[i][1])/(vertices[j][1]-vertices[i][1])*(vertices[j][0]-vertices[i][0]));
			}
			j=i;
		}
		//  Sort the nodes, via a simple “Bubble” sort.
		i=0;
		while (i<nodes-1) {
			if (nodeX[i]>nodeX[i+1]) {
				swap=nodeX[i]; nodeX[i]=nodeX[i+1]; nodeX[i+1]=swap; if (i) i--; }
			else {
				i++;
			}
		}
		//  Fill the pixels between node pairs.
		for (i=0; i<nodes; i+=2) {
			if (nodeX[i  ]>=IMAGE_RIGHT)
				break;
			if (nodeX[i+1]> IMAGE_LEFT ) {
				if (nodeX[i  ]< IMAGE_LEFT )
					nodeX[i  ]=IMAGE_LEFT ;
				if (nodeX[i+1]> IMAGE_RIGHT)
					nodeX[i+1]=IMAGE_RIGHT;
				for (pixelX=nodeX[i]; pixelX<nodeX[i+1]; pixelX++)
					SetPixel(dest, destWidth, pixelX, pixelY, color);
			}
		}
	}
}

#endif

int XAtlas_PrintCallback( const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Printf( "%s", msg );

	return 0;
}

// XYZ ST ST
struct vertexSig_t
{
	float x, y, z;
	float s1, t1, s2, t2; // s1 and s2 for warps?
};

// For export
struct vertexSigCompact_t
{
	float x, y, z;
	float s, t;
};

extern void R_BuildAtlasForModel( model_t *mod );

void R_FillOutShit_f()
{
	R_BuildAtlasForModel( r_worldmodel );
}

void R_BuildAtlasForModel( model_t *mod )
{
	if ( !mod )
		return;

	Com_Printf( "Building atlas for %s\n", mod->name );

#if 1
	uint32 totalVertices = 0;
	std::vector<vertexSigCompact_t> polyData;
	std::vector<uint8> faceCountData;
	for ( int i = 0; i < mod->numsurfaces; ++i )
	{
		msurface_t *surf = &mod->surfaces[i];
		if ( surf->texinfo->flags & SURFMASK_UNLIT )
			continue;

		glpoly_t *p = surf->polys;

		uint8 &count = faceCountData.emplace_back();
		count = p->numverts;
		totalVertices += p->numverts;

		for ( ; p != nullptr; p = p->next )
		{
			float *v = p->verts[0];
			for ( int j = 0; j < p->numverts; j++, v += VERTEXSIZE )
			{
				vertexSigCompact_t &newv = polyData.emplace_back();
				newv.x = v[2];
				newv.y = v[1];
				newv.z = v[0];
				newv.s = v[5];
				newv.t = v[6];
			}
		}
	}
#endif

	xatlas::SetPrint( XAtlas_PrintCallback, true );
	xatlas::Atlas *pAtlas = xatlas::Create();

	assert( mod->numsurfaces == faceCountData.size() );
	assert( totalVertices == polyData.size() );

	const xatlas::MeshDecl meshDecl
	{
		.vertexPositionData = (void *)polyData.data(),
		.vertexUvData = (void *)( (float *)( polyData.data() ) + 3 ),

		.faceVertexCount = faceCountData.data(),

		.vertexCount = (uint32)polyData.size(),
		.vertexPositionStride = sizeof( vertexSigCompact_t ),
		.vertexUvStride = sizeof( vertexSigCompact_t ),
		.faceCount = (uint32)faceCountData.size()
	};

	xatlas::AddMeshError meshError = xatlas::AddMesh( pAtlas, meshDecl );
	if ( meshError != xatlas::AddMeshError::Success ) {
		xatlas::Destroy( pAtlas );
		Com_Printf( "XAtlas failed: %s\n", xatlas::StringForEnum( meshError ) );
		return;
	}

	xatlas::ChartOptions chartOptions
	{
		.useInputMeshUvs = true,
		.fixWinding = true
	};

	xatlas::PackOptions packOptions
	{
		.rotateCharts = false
	};

	xatlas::Generate( pAtlas, chartOptions, packOptions );

	Com_Printf( "%d charts\n", pAtlas->chartCount );
	Com_Printf( "%d atlases\n", pAtlas->atlasCount );

	for ( uint32 i = 0; i < pAtlas->atlasCount; ++i )
		Com_Printf( "%u: %0.2f%% utilization\n", i, pAtlas->utilization[i] * 100.0f );
	Com_Printf( "%ux%u resolution\n", pAtlas->width, pAtlas->height );

#ifdef GARBAGE

#if 1
	// Write meshes.
	const char *modelFilename = "example_output.obj";
	printf("Writing '%s'...\n", modelFilename);
	FILE *file = fopen( modelFilename, "wb" );
	if (file)
	{
		uint32_t firstVertex = 0;
		for (uint32_t i = 0; i < pAtlas->meshCount; i++)
		{
			const xatlas::Mesh &mesh = pAtlas->meshes[i];
			for (uint32_t v = 0; v < mesh.vertexCount; v++)
			{
				const xatlas::Vertex &vertex = mesh.vertexArray[v];
				const float *pos = (float *)&polyData[vertex.xref];
				fprintf(file, "v %g %g %g\n", pos[0], pos[1], pos[2]);
				fprintf(file, "vt %g %g\n", vertex.uv[0] / pAtlas->width, 1.0f - (vertex.uv[1] / pAtlas->height));
			}
			fprintf(file, "o %s\n", mod->name);
			fprintf(file, "s off\n");

			uint32_t faceCount = faceCountData.size();
			uint32_t currentIndex = 0;
			for (uint32_t f = 0; f < faceCount; f++) {
				fprintf(file, "f ");
				uint32 faceVertexCount = faceCountData[f];
				for (uint32_t j = 0; j < faceVertexCount; j++) {
					const uint32_t index = firstVertex + mesh.indexArray[currentIndex++] + 1; // 1-indexed
					fprintf(file, "%d/%d/%d%c", index, index, index, j == (faceVertexCount - 1) ? '\n' : ' ');
				}
			}

			firstVertex += mesh.vertexCount;
		}
		fclose(file);
	}
#endif

#if 0
	if (pAtlas->width > 0 && pAtlas->height > 0) {
		printf("Rasterizing result...\n");
		// Dump images.
		std::vector<uint8_t> outputTrisImage, outputChartsImage;
		const uint32_t imageDataSize = pAtlas->width * pAtlas->height * 3;
		outputTrisImage.resize(pAtlas->atlasCount * imageDataSize);
		outputChartsImage.resize(pAtlas->atlasCount * imageDataSize);
		for (uint32_t i = 0; i < pAtlas->meshCount; i++) {
			const xatlas::Mesh &mesh = pAtlas->meshes[i];
			// Rasterize mesh triangles.
			const uint8_t white[] = { 255, 255, 255 };

			const uint32 faceCount = faceCountData.size();

			uint32_t faceFirstIndex = 0;
			for (uint32_t f = 0; f < faceCount; f++) {
				int32_t atlasIndex = -1;
				int verts[255][2];

				const uint32_t faceVertexCount = faceCountData[f];

				for (uint32_t j = 0; j < faceVertexCount; j++) {
					const xatlas::Vertex &v = mesh.vertexArray[mesh.indexArray[faceFirstIndex + j]];
					atlasIndex = v.atlasIndex; // The same for every vertex in the face.
					verts[j][0] = int(v.uv[0]);
					verts[j][1] = int(v.uv[1]);
				}
				if (atlasIndex < 0)
					continue; // Skip faces that weren't atlased.
				uint8_t color[3];
				RandomColor(color);
				uint8_t *imageData = &outputTrisImage[atlasIndex * imageDataSize];

				if (faceVertexCount == 3)
					RasterizeTriangle(imageData, pAtlas->width, verts[0], verts[1], verts[2], color);
				else
					RasterizePolygon(imageData, pAtlas->width, verts, (int)faceVertexCount, color);

				for (uint32_t j = 0; j < faceVertexCount; j++)
					RasterizeLine(imageData, pAtlas->width, verts[j], verts[(j + 1) % faceVertexCount], white);
				faceFirstIndex += faceVertexCount;
			}
			// Rasterize mesh charts.
			for (uint32_t j = 0; j < mesh.chartCount; j++) {
				const xatlas::Chart *chart = &mesh.chartArray[j];
				uint8_t color[3];
				RandomColor(color);
				for (uint32_t k = 0; k < chart->faceCount; k++) {
					const uint32_t face = chart->faceArray[k];

					const uint32_t faceVertexCount = faceCountData[face];
					faceFirstIndex = 0;
					for ( uint32_t l = 0; l < face; l++ )
						faceFirstIndex += faceCountData[l];

					int verts[255][2];
					for (uint32_t l = 0; l < faceVertexCount; l++) {
						const xatlas::Vertex &v = mesh.vertexArray[mesh.indexArray[faceFirstIndex + l]];
						verts[l][0] = int(v.uv[0]);
						verts[l][1] = int(v.uv[1]);
					}
					uint8_t *imageData = &outputChartsImage[chart->atlasIndex * imageDataSize];

					if (faceVertexCount == 3)
						RasterizeTriangle(imageData, pAtlas->width, verts[0], verts[1], verts[2], color);
					else
						RasterizePolygon(imageData, pAtlas->width, verts, (int)faceVertexCount, color);

					for (uint32_t l = 0; l < faceVertexCount; l++)
						RasterizeLine(imageData, pAtlas->width, verts[l], verts[(l + 1) % faceVertexCount], white);
				}
			}
		}
		for (uint32_t i = 0; i < pAtlas->atlasCount; i++) {
			char filename[256];
			Q_sprintf_s(filename, "example_tris%02u.png", i);
			printf("Writing '%s'...\n", filename);
			FILE *handle = fopen( filename, "wb" );
			img::WritePNG( pAtlas->width, pAtlas->height, false, &outputTrisImage[i * imageDataSize], handle );
			fclose( handle );
			Q_sprintf_s(filename, "example_charts%02u.png", i);
			printf("Writing '%s'...\n", filename);
			handle = fopen( filename, "wb" );
			img::WritePNG( pAtlas->width, pAtlas->height, false, &outputChartsImage[i * imageDataSize], handle );
			fclose( handle );
		}
	}
#endif

#endif

	xatlas::Destroy( pAtlas );
}
