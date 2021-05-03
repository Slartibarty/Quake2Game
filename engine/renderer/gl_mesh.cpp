// gl_mesh.c: triangle model functions

#include "gl_local.h"

#include <vector>

#define POWERSUIT_SCALE		4.0f

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

#define NUMVERTEXNORMALS	162

static float	r_avertexnormals[NUMVERTEXNORMALS][3]{
#include "anorms.inl"
};

static vec4_t	s_lerped[MAX_VERTS];

static vec3_t	shadevector;
static vec3_t	shadelight;

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
static float	r_avertexnormal_dots[SHADEDOT_QUANT][256]{
#include "anormtab.inl"
};

static float	*shadedots = r_avertexnormal_dots[0];

void GL_LerpVerts( int nverts, dtrivertx_t *v, dtrivertx_t *ov, dtrivertx_t *verts, float *lerp, float move[3], float frontv[3], float backv[3] )
{
	int i;

	//PMM -- added RF_SHELL_DOUBLE, RF_SHELL_HALF_DAM
	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) )
	{
		for (i=0 ; i < nverts; i++, v++, ov++, lerp+=4 )
		{
			float *normal = r_avertexnormals[verts[i].lightnormalindex];

			lerp[0] = move[0] + ov->v[0]*backv[0] + v->v[0]*frontv[0] + normal[0] * POWERSUIT_SCALE;
			lerp[1] = move[1] + ov->v[1]*backv[1] + v->v[1]*frontv[1] + normal[1] * POWERSUIT_SCALE;
			lerp[2] = move[2] + ov->v[2]*backv[2] + v->v[2]*frontv[2] + normal[2] * POWERSUIT_SCALE; 
		}
	}
	else
	{
		for (i=0 ; i < nverts; i++, v++, ov++, lerp+=4)
		{
			lerp[0] = move[0] + ov->v[0]*backv[0] + v->v[0]*frontv[0];
			lerp[1] = move[1] + ov->v[1]*backv[1] + v->v[1]*frontv[1];
			lerp[2] = move[2] + ov->v[2]*backv[2] + v->v[2]*frontv[2];
		}
	}

}

/*
=============
GL_DrawAliasFrameLerp

interpolates between two frames and origins
FIXME: batch lerp all vertexes
=============
*/
void GL_DrawAliasFrameLerp (dmdl_t *paliashdr, float backlerp)
{
	float 	l;
	daliasframe_t	*frame, *oldframe;
	dtrivertx_t	*v, *ov, *verts;
	int		*order;
	int		count;
	float	frontlerp;
	float	alpha;
	vec3_t	move, delta, vectors[3];
	vec3_t	frontv, backv;
	int		i;
	int		index_xyz;
	float	*lerp;

	frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames 
		+ currententity->frame * paliashdr->framesize);
	verts = v = frame->verts;

	oldframe = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames 
		+ currententity->oldframe * paliashdr->framesize);
	ov = oldframe->verts;

	order = (int *)((byte *)paliashdr + paliashdr->ofs_glcmds);

//	glTranslatef (frame->translate[0], frame->translate[1], frame->translate[2]);
//	glScalef (frame->scale[0], frame->scale[1], frame->scale[2]);

	if (currententity->flags & RF_TRANSLUCENT)
		alpha = currententity->alpha;
	else
		alpha = 1.0f;

	// PMM - added double shell
	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) )
		glDisable( GL_TEXTURE_2D );

	frontlerp = 1.0f - backlerp;

	// move should be the delta back to the previous frame * backlerp
	VectorSubtract (currententity->oldorigin, currententity->origin, delta);
	AngleVectors (currententity->angles, vectors[0], vectors[1], vectors[2]);

	move[0] = DotProduct (delta, vectors[0]);	// forward
	move[1] = -DotProduct (delta, vectors[1]);	// left
	move[2] = DotProduct (delta, vectors[2]);	// up

	VectorAdd (move, oldframe->translate, move);

	for (i=0 ; i<3 ; i++)
	{
		move[i] = backlerp*move[i] + frontlerp*frame->translate[i];
	}

	for (i=0 ; i<3 ; i++)
	{
		frontv[i] = frontlerp*frame->scale[i];
		backv[i] = backlerp*oldframe->scale[i];
	}

	lerp = s_lerped[0];

	GL_LerpVerts( paliashdr->num_xyz, v, ov, verts, lerp, move, frontv, backv );

	if ( r_vertex_arrays->value )
	{
		static float colorArray[MAX_VERTS*4];

		glEnableClientState( GL_VERTEX_ARRAY );
		glVertexPointer( 3, GL_FLOAT, 16, s_lerped );	// padded for SIMD

//		if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE ) )
		// PMM - added double damage shell
		if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) )
		{
			glColor4f( shadelight[0], shadelight[1], shadelight[2], alpha );
		}
		else
		{
			glEnableClientState( GL_COLOR_ARRAY );
			glColorPointer( 3, GL_FLOAT, 0, colorArray );

			//
			// pre light everything
			//
			for ( i = 0; i < paliashdr->num_xyz; i++ )
			{
				float l = shadedots[verts[i].lightnormalindex];

				colorArray[i*3+0] = l * shadelight[0];
				colorArray[i*3+1] = l * shadelight[1];
				colorArray[i*3+2] = l * shadelight[2];
			}
		}

		if ( GLEW_EXT_compiled_vertex_array && r_ext_compiled_vertex_array->value )
			glLockArraysEXT( 0, paliashdr->num_xyz );

		while (1)
		{
			// get the vertex count and primitive type
			count = *order++;
			if (!count)
				break;		// done
			if (count < 0)
			{
				count = -count;
				glBegin (GL_TRIANGLE_FAN);
			}
			else
			{
				glBegin (GL_TRIANGLE_STRIP);
			}

			// PMM - added double damage shell
			if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) )
			{
				do
				{
					index_xyz = order[2];
					order += 3;

					glVertex3fv( s_lerped[index_xyz] );

				} while (--count);
			}
			else
			{
				do
				{
					// texture coordinates come from the draw list
					glTexCoord2f (((float *)order)[0], ((float *)order)[1]);
					index_xyz = order[2];

					order += 3;

					// normals and vertexes come from the frame list
//					l = shadedots[verts[index_xyz].lightnormalindex];
					
//					glColor4f (l* shadelight[0], l*shadelight[1], l*shadelight[2], alpha);
					glArrayElement( index_xyz );

				} while (--count);
			}
			glEnd ();
		}

		if ( GLEW_EXT_compiled_vertex_array && r_ext_compiled_vertex_array->value )
			glUnlockArraysEXT();
	}
	else
	{
		while (1)
		{
			// get the vertex count and primitive type
			count = *order++;
			if (!count)
				break;		// done
			if (count < 0)
			{
				count = -count;
				glBegin (GL_TRIANGLE_FAN);
			}
			else
			{
				glBegin (GL_TRIANGLE_STRIP);
			}

			if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE ) )
			{
				do
				{
					index_xyz = order[2];
					order += 3;

					glColor4f( shadelight[0], shadelight[1], shadelight[2], alpha);
					glVertex3fv (s_lerped[index_xyz]);

				} while (--count);
			}
			else
			{
				do
				{
					// texture coordinates come from the draw list
					glTexCoord2f (((float *)order)[0], ((float *)order)[1]);
					index_xyz = order[2];
					order += 3;

					// normals and vertexes come from the frame list
					l = shadedots[verts[index_xyz].lightnormalindex];
					
					glColor4f (l* shadelight[0], l*shadelight[1], l*shadelight[2], alpha);
					glVertex3fv (s_lerped[index_xyz]);
				} while (--count);
			}

			glEnd ();
		}
	}

//	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE ) )
	// PMM - added double damage shell
	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) )
		glEnable( GL_TEXTURE_2D );
}

/*
** R_CullAliasModel
*/
static bool R_CullAliasModel( vec3_t bbox[8], entity_t *e )
{
	int i;
	vec3_t		mins, maxs;
	dmdl_t		*paliashdr;
	vec3_t		vectors[3];
	vec3_t		thismins, oldmins, thismaxs, oldmaxs;
	daliasframe_t *pframe, *poldframe;
	vec3_t angles;

	paliashdr = (dmdl_t *)currentmodel->extradata;

	if ( ( e->frame >= paliashdr->num_frames ) || ( e->frame < 0 ) )
	{
		Com_Printf("R_CullAliasModel %s: no such frame %d\n", 
			currentmodel->name, e->frame);
		e->frame = 0;
	}
	if ( ( e->oldframe >= paliashdr->num_frames ) || ( e->oldframe < 0 ) )
	{
		Com_Printf("R_CullAliasModel %s: no such oldframe %d\n", 
			currentmodel->name, e->oldframe);
		e->oldframe = 0;
	}

	pframe = ( daliasframe_t * ) ( ( byte * ) paliashdr + 
									  paliashdr->ofs_frames +
									  e->frame * paliashdr->framesize);

	poldframe = ( daliasframe_t * ) ( ( byte * ) paliashdr + 
									  paliashdr->ofs_frames +
									  e->oldframe * paliashdr->framesize);

	/*
	** compute axially aligned mins and maxs
	*/
	if ( pframe == poldframe )
	{
		for ( i = 0; i < 3; i++ )
		{
			mins[i] = pframe->translate[i];
			maxs[i] = mins[i] + pframe->scale[i]*255;
		}
	}
	else
	{
		for ( i = 0; i < 3; i++ )
		{
			thismins[i] = pframe->translate[i];
			thismaxs[i] = thismins[i] + pframe->scale[i]*255;

			oldmins[i]  = poldframe->translate[i];
			oldmaxs[i]  = oldmins[i] + poldframe->scale[i]*255;

			if ( thismins[i] < oldmins[i] )
				mins[i] = thismins[i];
			else
				mins[i] = oldmins[i];

			if ( thismaxs[i] > oldmaxs[i] )
				maxs[i] = thismaxs[i];
			else
				maxs[i] = oldmaxs[i];
		}
	}

	/*
	** compute a full bounding box
	*/
	for ( i = 0; i < 8; i++ )
	{
		vec3_t   tmp;

		if ( i & 1 )
			tmp[0] = mins[0];
		else
			tmp[0] = maxs[0];

		if ( i & 2 )
			tmp[1] = mins[1];
		else
			tmp[1] = maxs[1];

		if ( i & 4 )
			tmp[2] = mins[2];
		else
			tmp[2] = maxs[2];

		VectorCopy( tmp, bbox[i] );
	}

	/*
	** rotate the bounding box
	*/
	VectorCopy( e->angles, angles );
	angles[YAW] = -angles[YAW];
	AngleVectors( angles, vectors[0], vectors[1], vectors[2] );

	for ( i = 0; i < 8; i++ )
	{
		vec3_t tmp;

		VectorCopy( bbox[i], tmp );

		bbox[i][0] = DotProduct( vectors[0], tmp );
		bbox[i][1] = -DotProduct( vectors[1], tmp );
		bbox[i][2] = DotProduct( vectors[2], tmp );

		VectorAdd( e->origin, bbox[i], bbox[i] );
	}

	{
		int p, f, aggregatemask = ~0;

		for ( p = 0; p < 8; p++ )
		{
			int mask = 0;

			for ( f = 0; f < 4; f++ )
			{
				float dp = DotProduct( frustum[f].normal, bbox[p] );

				if ( ( dp - frustum[f].dist ) < 0 )
				{
					mask |= ( 1 << f );
				}
			}

			aggregatemask &= mask;
		}

		if ( aggregatemask )
		{
			return true;
		}

		return false;
	}
}

/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel (entity_t *e)
{
	int			i;
	dmdl_t		*paliashdr;
	float		an;
	vec3_t		bbox[8];
	material_t	*skin;

	if ( !( e->flags & RF_WEAPONMODEL ) )
	{
		if ( R_CullAliasModel( bbox, e ) )
			return;
	}

	if ( e->flags & RF_WEAPONMODEL )
	{
		if ( r_lefthand->value == 2 )
			return;
	}

	paliashdr = (dmdl_t *)currentmodel->extradata;

	//
	// get lighting information
	//
	// PMM - rewrote, reordered to handle new shells & mixing
	// PMM - 3.20 code .. replaced with original way of doing it to keep mod authors happy
	//
	if ( currententity->flags & ( RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE ) )
	{
		VectorClear (shadelight);
		if (currententity->flags & RF_SHELL_HALF_DAM)
		{
				shadelight[0] = 0.56;
				shadelight[1] = 0.59;
				shadelight[2] = 0.45;
		}
		if ( currententity->flags & RF_SHELL_DOUBLE )
		{
			shadelight[0] = 0.9;
			shadelight[1] = 0.7;
		}
		if ( currententity->flags & RF_SHELL_RED )
			shadelight[0] = 1.0;
		if ( currententity->flags & RF_SHELL_GREEN )
			shadelight[1] = 1.0;
		if ( currententity->flags & RF_SHELL_BLUE )
			shadelight[2] = 1.0;
	}
/*
		// PMM -special case for godmode
		if ( (currententity->flags & RF_SHELL_RED) &&
			(currententity->flags & RF_SHELL_BLUE) &&
			(currententity->flags & RF_SHELL_GREEN) )
		{
			for (i=0 ; i<3 ; i++)
				shadelight[i] = 1.0;
		}
		else if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE ) )
		{
			VectorClear (shadelight);

			if ( currententity->flags & RF_SHELL_RED )
			{
				shadelight[0] = 1.0;
				if (currententity->flags & (RF_SHELL_BLUE|RF_SHELL_DOUBLE) )
					shadelight[2] = 1.0;
			}
			else if ( currententity->flags & RF_SHELL_BLUE )
			{
				if ( currententity->flags & RF_SHELL_DOUBLE )
				{
					shadelight[1] = 1.0;
					shadelight[2] = 1.0;
				}
				else
				{
					shadelight[2] = 1.0;
				}
			}
			else if ( currententity->flags & RF_SHELL_DOUBLE )
			{
				shadelight[0] = 0.9;
				shadelight[1] = 0.7;
			}
		}
		else if ( currententity->flags & ( RF_SHELL_HALF_DAM | RF_SHELL_GREEN ) )
		{
			VectorClear (shadelight);
			// PMM - new colors
			if ( currententity->flags & RF_SHELL_HALF_DAM )
			{
				shadelight[0] = 0.56;
				shadelight[1] = 0.59;
				shadelight[2] = 0.45;
			}
			if ( currententity->flags & RF_SHELL_GREEN )
			{
				shadelight[1] = 1.0;
			}
		}
	}
			//PMM - ok, now flatten these down to range from 0 to 1.0.
	//		max_shell_val = max(shadelight[0], max(shadelight[1], shadelight[2]));
	//		if (max_shell_val > 0)
	//		{
	//			for (i=0; i<3; i++)
	//			{
	//				shadelight[i] = shadelight[i] / max_shell_val;
	//			}
	//		}
	// pmm
*/
	else if ( currententity->flags & RF_FULLBRIGHT )
	{
		for (i=0 ; i<3 ; i++)
			shadelight[i] = 1.0;
	}
	else
	{
		R_LightPoint (currententity->origin, shadelight);

		// player lighting hack for communication back to server
		// big hack!
		if ( currententity->flags & RF_WEAPONMODEL )
		{
			// pick the greatest component, which should be the same
			// as the mono value returned by software
			if (shadelight[0] > shadelight[1])
			{
				if (shadelight[0] > shadelight[2])
					r_lightlevel->value = 150*shadelight[0];
				else
					r_lightlevel->value = 150*shadelight[2];
			}
			else
			{
				if (shadelight[1] > shadelight[2])
					r_lightlevel->value = 150*shadelight[1];
				else
					r_lightlevel->value = 150*shadelight[2];
			}

		}
	}

	if ( currententity->flags & RF_MINLIGHT )
	{
		for (i=0 ; i<3 ; i++)
			if (shadelight[i] > 0.1f)
				break;
		if (i == 3)
		{
			shadelight[0] = 0.1f;
			shadelight[1] = 0.1f;
			shadelight[2] = 0.1f;
		}
	}

	if ( currententity->flags & RF_GLOW )
	{	// bonus items will pulse with time
		float	scale;
		float	min;

		scale = 0.1f * sin(r_newrefdef.time*7);
		for (i=0 ; i<3 ; i++)
		{
			min = shadelight[i] * 0.8f;
			shadelight[i] += scale;
			if (shadelight[i] < min)
				shadelight[i] = min;
		}
	}

// =================
// PGM	ir goggles color override
	if ( r_newrefdef.rdflags & RDF_IRGOGGLES && currententity->flags & RF_IR_VISIBLE)
	{
		shadelight[0] = 1.0f;
		shadelight[1] = 0.0f;
		shadelight[2] = 0.0f;
	}
// PGM	
// =================

	shadedots = r_avertexnormal_dots[((int)(currententity->angles[1] * (SHADEDOT_QUANT / 360.0f))) & (SHADEDOT_QUANT - 1)];
	
	an = RAD2DEG( currententity->angles[1] );
	shadevector[0] = cos(-an);
	shadevector[1] = sin(-an);
	shadevector[2] = 1;
	VectorNormalize (shadevector);

	//
	// locate the proper data
	//

	c_alias_polys += paliashdr->num_tris;

	//
	// draw all the triangles
	//
	if (currententity->flags & RF_DEPTHHACK) // hack the depth range to prevent view model from poking into walls
		glDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));

	if ( ( currententity->flags & RF_WEAPONMODEL ) && ( r_lefthand->value == 1.0F ) )
	{
		extern void MYgluPerspective( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar );

		glMatrixMode( GL_PROJECTION );
		glPushMatrix();
		glLoadIdentity();
		glScalef( -1, 1, 1 );
		MYgluPerspective( r_newrefdef.fov_y, ( float ) r_newrefdef.width / r_newrefdef.height,  4,  4096);
		glMatrixMode( GL_MODELVIEW );

		glCullFace( GL_BACK );
	}

	glPushMatrix ();
	e->angles[PITCH] = -e->angles[PITCH];	// sigh.
	R_RotateForEntity (e);
	e->angles[PITCH] = -e->angles[PITCH];	// sigh.

	// select skin
	if (currententity->skin)
		skin = currententity->skin;	// custom player skin
	else
	{
		if (currententity->skinnum >= MAX_MD2SKINS)
			skin = currentmodel->skins[0];
		else
		{
			skin = currentmodel->skins[currententity->skinnum];
			if (!skin)
				skin = currentmodel->skins[0];
		}
	}
	if (!skin)
		skin = mat_notexture;	// fallback...
	skin->Bind();

	// draw it

	glShadeModel (GL_SMOOTH);

	GL_TexEnv( GL_MODULATE );
	if ( currententity->flags & RF_TRANSLUCENT )
	{
		glEnable (GL_BLEND);
	}


	if ( (currententity->frame >= paliashdr->num_frames) 
		|| (currententity->frame < 0) )
	{
		Com_Printf("R_DrawAliasModel %s: no such frame %d\n",
			currentmodel->name, currententity->frame);
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if ( (currententity->oldframe >= paliashdr->num_frames)
		|| (currententity->oldframe < 0))
	{
		Com_Printf("R_DrawAliasModel %s: no such oldframe %d\n",
			currentmodel->name, currententity->oldframe);
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if ( !r_lerpmodels->value )
		currententity->backlerp = 0;
	GL_DrawAliasFrameLerp (paliashdr, currententity->backlerp);

	GL_TexEnv( GL_REPLACE );
	glShadeModel (GL_FLAT);

	glPopMatrix ();

#if 0
	glDisable( GL_CULL_FACE );
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glDisable( GL_TEXTURE_2D );
	glBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i < 8; i++ )
	{
		glVertex3fv( bbox[i] );
	}
	glEnd();
	glEnable( GL_TEXTURE_2D );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glEnable( GL_CULL_FACE );
#endif

	if ( ( currententity->flags & RF_WEAPONMODEL ) && ( r_lefthand->value == 1.0F ) )
	{
		glMatrixMode( GL_PROJECTION );
		glPopMatrix();
		glMatrixMode( GL_MODELVIEW );
		glCullFace( GL_FRONT );
	}

	if ( currententity->flags & RF_TRANSLUCENT )
	{
		glDisable (GL_BLEND);
	}

	if (currententity->flags & RF_DEPTHHACK)
		glDepthRange (gldepthmin, gldepthmax);

	glColor4f (1,1,1,1);
}

#if 1

/*
=============================================================

  STUDIO MODELS

=============================================================
*/

static constexpr float StudioLambert = 1.5f;

vec3_t			g_xformverts[MAXSTUDIOVERTS];	// transformed vertices
vec3_t			g_lightvalues[MAXSTUDIOVERTS];	// light surface normals
vec3_t			*g_pxformverts;
vec3_t			*g_pvlightvalues;

vec3_t			g_lightvec;						// light vector in model reference frame
vec3_t			g_blightvec[MAXSTUDIOBONES];	// light vectors in bone reference frames
int				g_ambientlight;					// ambient world light
float			g_shadelight;					// direct world light
vec3_t			g_lightcolor;

int				g_smodels_total;				// cookie

float			g_bonetransform[MAXSTUDIOBONES][3][4];	// bone transformation matrix

int				g_chrome[MAXSTUDIOVERTS][2];	// texture coords for surface normals
int				g_chromeage[MAXSTUDIOBONES];	// last time chrome vectors were updated
vec3_t			g_chromeup[MAXSTUDIOBONES];		// chrome vector "up" in bone reference frames
vec3_t			g_chromeright[MAXSTUDIOBONES];	// chrome vector "right" in bone reference frames

int				m_bodynum;			// bodypart selection
vec4_t			m_adj;				// FIX: non persistant, make static
byte			m_controller[4];	// bone controllers
byte			m_blending[2];		// animation blending
byte			m_mouth;			// mouth position
int				m_sequence = 5;		// sequence index
int				m_skinnum = 0;
float			m_frame = 0;

static void Studio_CalcBoneAdj( studiohdr_t *phdr )
{
	int					i, j;
	float				value;
	mstudiobonecontroller_t *pbonecontroller;
	
	pbonecontroller = (mstudiobonecontroller_t *)((byte *)phdr + phdr->bonecontrollerindex);

	for (j = 0; j < phdr->numbonecontrollers; j++)
	{
		i = pbonecontroller[j].index;
		if (i <= 3)
		{
			// check for 360% wrapping
			if (pbonecontroller[j].type & STUDIO_RLOOP)
			{
				value = m_controller[i] * (360.0f/256.0f) + pbonecontroller[j].start;
			}
			else 
			{
				value = m_controller[i] / 255.0f;
				if (value < 0.0f) value = 0.0f;
				if (value > 1.0f) value = 1.0f;
				value = (1.0f - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
			}
		//	Com_DPrintf( "%d %d %f : %f\n", m_controller[j], m_prevcontroller[j], value, dadt );
		}
		else
		{
			value = m_mouth / 64.0f;
			if (value > 1.0f) value = 1.0f;
			value = (1.0f - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
		//	Com_DPrintf( "%d %f\n", mouthopen, value );
		}
		switch(pbonecontroller[j].type & STUDIO_TYPES)
		{
		case STUDIO_XR:
		case STUDIO_YR:
		case STUDIO_ZR:
			m_adj[j] = DEG2RAD( value );
			break;
		case STUDIO_X:
		case STUDIO_Y:
		case STUDIO_Z:
			m_adj[j] = value;
			break;
		}
	}
}

static void Studio_CalcBoneQuaternion( int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *q )
{
	int					j, k;
	vec4_t				q1, q2;
	vec3_t				angle1, angle2;
	mstudioanimvalue_t	*panimvalue;

	for (j = 0; j < 3; j++)
	{
		if (panim->offset[j+3] == 0)
		{
			angle2[j] = angle1[j] = pbone->value[j+3]; // default;
		}
		else
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j+3]);
			k = frame;
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
			}
			// Bah, missing blend!
			if (panimvalue->num.valid > k)
			{
				angle1[j] = panimvalue[k+1].value;

				if (panimvalue->num.valid > k + 1)
				{
					angle2[j] = panimvalue[k+2].value;
				}
				else
				{
					if (panimvalue->num.total > k + 1)
						angle2[j] = angle1[j];
					else
						angle2[j] = panimvalue[panimvalue->num.valid+2].value;
				}
			}
			else
			{
				angle1[j] = panimvalue[panimvalue->num.valid].value;
				if (panimvalue->num.total > k + 1)
				{
					angle2[j] = angle1[j];
				}
				else
				{
					angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}
			angle1[j] = pbone->value[j+3] + angle1[j] * pbone->scale[j+3];
			angle2[j] = pbone->value[j+3] + angle2[j] * pbone->scale[j+3];
		}

		if (pbone->bonecontroller[j+3] != -1)
		{
			angle1[j] += m_adj[pbone->bonecontroller[j+3]];
			angle2[j] += m_adj[pbone->bonecontroller[j+3]];
		}
	}

	if (!VectorCompare( angle1, angle2 ))
	{
		AngleQuaternion( angle1, q1 );
		AngleQuaternion( angle2, q2 );
		QuaternionSlerp( q1, q2, s, q );
	}
	else
	{
		AngleQuaternion( angle1, q );
	}
}


static void Studio_CalcBonePosition( int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *pos )
{
	int					j, k;
	mstudioanimvalue_t	*panimvalue;

	for (j = 0; j < 3; j++)
	{
		pos[j] = pbone->value[j]; // default;
		if (panim->offset[j] != 0)
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j]);
			
			k = frame;
			// find span of values that includes the frame we want
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
			}
			// if we're inside the span
			if (panimvalue->num.valid > k)
			{
				// and there's more data in the span
				if (panimvalue->num.valid > k + 1)
				{
					pos[j] += (panimvalue[k+1].value * (1.0f - s) + s * panimvalue[k+2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[k+1].value * pbone->scale[j];
				}
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if (panimvalue->num.total <= k + 1)
				{
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0f - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
				}
			}
		}
		if (pbone->bonecontroller[j] != -1)
		{
			pos[j] += m_adj[pbone->bonecontroller[j]];
		}
	}
}


static void Studio_CalcRotations ( vec3_t *pos, vec4_t *q, studiohdr_t *phdr, mstudioseqdesc_t *pseqdesc, mstudioanim_t *panim, float f )
{
	int					i;
	int					frame;
	mstudiobone_t		*pbone;
	float				s;

	frame = (int)f;
	s = (f - frame);

	// add in programatic controllers
	Studio_CalcBoneAdj( phdr );

	pbone		= (mstudiobone_t *)((byte *)phdr + phdr->boneindex);
	for (i = 0; i < phdr->numbones; i++, pbone++, panim++)
	{
		Studio_CalcBoneQuaternion( frame, s, pbone, panim, q[i] );
		Studio_CalcBonePosition( frame, s, pbone, panim, pos[i] );
	}

	if (pseqdesc->motiontype & STUDIO_X)
		pos[pseqdesc->motionbone][0] = 0.0f;
	if (pseqdesc->motiontype & STUDIO_Y)
		pos[pseqdesc->motionbone][1] = 0.0f;
	if (pseqdesc->motiontype & STUDIO_Z)
		pos[pseqdesc->motionbone][2] = 0.0f;
}

static mstudioanim_t *Studio_GetAnim( studiohdr_t *phdr, mstudioseqdesc_t *pseqdesc )
{
	mstudioseqgroup_t	*pseqgroup;
	pseqgroup = (mstudioseqgroup_t *)((byte *)phdr + phdr->seqgroupindex) + pseqdesc->seqgroup;

	if (pseqdesc->seqgroup == 0)
	{
		return (mstudioanim_t *)((byte *)phdr + pseqgroup->unused2 /* was pseqgroup->data, will be almost always be 0 */ + pseqdesc->animindex);
	}

//	return (mstudioanim_t *)((byte *)m_panimhdr[pseqdesc->seqgroup] + pseqdesc->animindex);
	return nullptr;
}

static void Studio_SlerpBones( studiohdr_t *phdr, vec4_t q1[], vec3_t pos1[], vec4_t q2[], vec3_t pos2[], float s )
{
	int			i;
	vec4_t		q3;
	float		s1;

	s = Clamp( s, 0.0f, 1.0f );

	s1 = 1.0f - s;

	for (i = 0; i < phdr->numbones; i++)
	{
		QuaternionSlerp( q1[i], q2[i], s, q3 );
		q1[i][0] = q3[0];
		q1[i][1] = q3[1];
		q1[i][2] = q3[2];
		q1[i][3] = q3[3];
		pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s;
		pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s;
		pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s;
	}
}

static void Studio_AdvanceFrame( studiohdr_t *phdr, float dt )
{	
	mstudioseqdesc_t *pseqdesc;
	pseqdesc = (mstudioseqdesc_t *)((byte *)phdr + phdr->seqindex) + m_sequence;

	if ( dt > 0.1f ) {
		dt = 0.1f;
	}
	m_frame += dt * pseqdesc->fps;

	if (pseqdesc->numframes <= 1)
	{
		m_frame = 0.0f;
	}
	else
	{
		// wrap
		m_frame -= (int)(m_frame / (pseqdesc->numframes - 1)) * (pseqdesc->numframes - 1);
	}
}

static void Studio_SetUpBones( studiohdr_t *phdr )
{
	int					i;

	mstudiobone_t		*pbones;
	mstudioseqdesc_t	*pseqdesc;
	mstudioanim_t		*panim;

	vec3_t				pos[MAXSTUDIOBONES];
	float				bonematrix[3][4];
	vec4_t				q[MAXSTUDIOBONES];

	vec3_t				pos2[MAXSTUDIOBONES];
	vec4_t				q2[MAXSTUDIOBONES];
	vec3_t				pos3[MAXSTUDIOBONES];
	vec4_t				q3[MAXSTUDIOBONES];
	vec3_t				pos4[MAXSTUDIOBONES];
	vec4_t				q4[MAXSTUDIOBONES];


	if (m_sequence >= phdr->numseq) {
		m_sequence = 0;
	}

	pseqdesc = (mstudioseqdesc_t *)((byte *)phdr + phdr->seqindex) + m_sequence;

	panim = Studio_GetAnim( phdr, pseqdesc );
	assert( panim );
	Studio_CalcRotations( pos, q, phdr, pseqdesc, panim, m_frame );

	if (pseqdesc->numblends > 1)
	{
		float s;

		panim += phdr->numbones;
		Studio_CalcRotations( pos2, q2, phdr, pseqdesc, panim, m_frame );
		s = m_blending[0] / 255.0f;

		Studio_SlerpBones( phdr, q, pos, q2, pos2, s );

		if (pseqdesc->numblends == 4)
		{
			panim += phdr->numbones;
			Studio_CalcRotations( pos3, q3, phdr, pseqdesc, panim, m_frame );

			panim += phdr->numbones;
			Studio_CalcRotations( pos4, q4, phdr, pseqdesc, panim, m_frame );

			s = m_blending[0] / 255.0f;
			Studio_SlerpBones( phdr, q3, pos3, q4, pos4, s );

			s = m_blending[1] / 255.0f;
			Studio_SlerpBones( phdr, q, pos, q3, pos3, s );
		}
	}

	pbones = (mstudiobone_t *)((byte *)phdr + phdr->boneindex);

	for (i = 0; i < phdr->numbones; i++) {
		QuaternionMatrix( q[i], bonematrix );

		bonematrix[0][3] = pos[i][0];
		bonematrix[1][3] = pos[i][1];
		bonematrix[2][3] = pos[i][2];

		if (pbones[i].parent == -1) {
			memcpy(g_bonetransform[i], bonematrix, sizeof(float) * 12);
		} 
		else {
			R_ConcatTransforms (g_bonetransform[pbones[i].parent], bonematrix, g_bonetransform[i]);
		}
	}
}

static void Studio_Lighting( float *lv, int bone, int flags, vec3_t normal )
{
	float 	illum;
	float	lightcos;

	illum = g_ambientlight;

	if ( flags & STUDIO_NF_FLATSHADE )
	{
		illum += g_shadelight * 0.8f;
	}
	else
	{
		float r;
		lightcos = DotProduct( normal, g_blightvec[bone] ); // -1 colinear, 1 opposite

		if ( lightcos > 1.0f )
			lightcos = 1.0f;

		illum += g_shadelight;

		r = StudioLambert;
		if ( r <= 1.0f ) r = 1.0f;

		lightcos = ( lightcos + ( r - 1.0f ) ) / r; 		// do modified hemispherical lighting
		if ( lightcos > 0.0f )
		{
			illum -= g_shadelight * lightcos;
		}
		if ( illum <= 0.0f )
			illum = 0.0f;
	}

	if ( illum > 255.0f )
		illum = 255.0f;
	*lv = illum / 255.0f;	// Light from 0 to 1.0
}

static void Studio_Chrome( int *pchrome, int bone, vec3_t origin, vec3_t normal )
{
	float n;

	if ( g_chromeage[bone] != g_smodels_total )
	{
		// calculate vectors from the viewer to the bone. This roughly adjusts for position
		vec3_t chromeupvec;		// g_chrome t vector in world reference frame
		vec3_t chromerightvec;	// g_chrome s vector in world reference frame
		vec3_t tmp;				// vector pointing at bone in world reference frame
		VectorScale( origin, -1, tmp );
		tmp[0] += g_bonetransform[bone][0][3];
		tmp[1] += g_bonetransform[bone][1][3];
		tmp[2] += g_bonetransform[bone][2][3];
		VectorNormalize( tmp );
		CrossProduct( tmp, vright, chromeupvec );
		VectorNormalize( chromeupvec );
		CrossProduct( tmp, chromeupvec, chromerightvec );
		VectorNormalize( chromerightvec );

		VectorIRotate( chromeupvec, g_bonetransform[bone], g_chromeup[bone] );
		VectorIRotate( chromerightvec, g_bonetransform[bone], g_chromeright[bone] );

		g_chromeage[bone] = g_smodels_total;
	}

	// calc s coord
	n = DotProduct( normal, g_chromeright[bone] );
	pchrome[0] = ( n + 1.0f ) * 32; // FIX: make this a float

	// calc t coord
	n = DotProduct( normal, g_chromeup[bone] );
	pchrome[1] = ( n + 1.0f ) * 32; // FIX: make this a float
}

static void Studio_SetupLighting( studiohdr_t *phdr )
{
	int i;
	g_ambientlight = 32;
	g_shadelight = 192.0f;

	g_lightvec[0] = 0.0f;
	g_lightvec[1] = 0.0f;
	g_lightvec[2] = -1.0f;

	g_lightcolor[0] = 1.0f;
	g_lightcolor[1] = 1.0f;
	g_lightcolor[2] = 1.0f;

	// TODO: only do it for bones that actually have textures
	for ( i = 0; i < phdr->numbones; i++ )
	{
		VectorIRotate( g_lightvec, g_bonetransform[i], g_blightvec[i] );
	}
}

static void Studio_DrawPoints( studiohdr_t *phdr, studiohdr_t *ptexturehdr, mstudiomodel_t *pmodel, vec3_t origin )
{
	int					i, j;
	mstudiomesh_t		*pmesh;
	byte				*pvertbone;
	byte				*pnormbone;
	vec3_t				*pstudioverts;
	vec3_t				*pstudionorms;
	mstudiotexture_t	*ptexture;
	float 				*av;
	float				*lv;
	float				lv_tmp;
	short				*pskinref;

	pvertbone = ((byte *)phdr + pmodel->vertinfoindex);
	pnormbone = ((byte *)phdr + pmodel->norminfoindex);
	ptexture = (mstudiotexture_t *)((byte *)ptexturehdr + ptexturehdr->textureindex);

	pmesh = (mstudiomesh_t *)((byte *)phdr + pmodel->meshindex);

	pstudioverts = (vec3_t *)((byte *)phdr + pmodel->vertindex);
	pstudionorms = (vec3_t *)((byte *)phdr + pmodel->normindex);

	pskinref = (short *)((byte *)ptexturehdr + ptexturehdr->skinindex);
	if ( m_skinnum != 0 && m_skinnum < ptexturehdr->numskinfamilies ) {
		pskinref += ( m_skinnum * ptexturehdr->numskinref );
	}

	for (i = 0; i < pmodel->numverts; i++)
	{
		VectorTransform (pstudioverts[i], g_bonetransform[pvertbone[i]], g_pxformverts[i]);
	}

//
// clip and draw all triangles
//
	lv = (float *)g_pvlightvalues;
	for (j = 0; j < pmodel->nummesh; j++) 
	{
		int flags = 0;
		flags = ptexture[pskinref[pmesh[j].skinref]].flags;
		for (i = 0; i < pmesh[j].numnorms; i++, lv += 3, pstudionorms++, pnormbone++)
		{
			Studio_Lighting (&lv_tmp, *pnormbone, flags, (float *)pstudionorms);

			// FIX: move this check out of the inner loop
			if (flags & STUDIO_NF_CHROME)
				Studio_Chrome( g_chrome[(float (*)[3])lv - g_pvlightvalues], *pnormbone, origin, (float *)pstudionorms );

			lv[0] = lv_tmp * g_lightcolor[0];
			lv[1] = lv_tmp * g_lightcolor[1];
			lv[2] = lv_tmp * g_lightcolor[2];
		}
	}

//	glCullFace(GL_FRONT);

	for (j = 0; j < pmodel->nummesh; j++)
	{
		float	s, t;
		short	*ptricmds;

		pmesh = (mstudiomesh_t *)((byte *)phdr + pmodel->meshindex) + j;
		ptricmds = (short *)((byte *)phdr + pmesh->triindex);

		s = 1.0f / (float)ptexture[pskinref[pmesh->skinref]].width;
		t = 1.0f / (float)ptexture[pskinref[pmesh->skinref]].height;

		GL_Bind( ptexture[pskinref[pmesh->skinref]].index );

		if (ptexture[pskinref[pmesh->skinref]].flags & STUDIO_NF_CHROME)
		{
			while (i = *(ptricmds++))
			{
				if (i < 0)
				{
					glBegin( GL_TRIANGLE_FAN );
					i = -i;
				}
				else
				{
					glBegin( GL_TRIANGLE_STRIP );
				}

				for( ; i > 0; i--, ptricmds += 4)
				{
					// FIX: put these in as integer coords, not floats
					glTexCoord2f(g_chrome[ptricmds[1]][0]*s, g_chrome[ptricmds[1]][1]*t);
					
					lv = g_pvlightvalues[ptricmds[1]];
					glColor4f( lv[0], lv[1], lv[2], 1.0 );

					av = g_pxformverts[ptricmds[0]];
					glVertex3f(av[0], av[1], av[2]);
				}
				glEnd( );
			}	
		} 
		else 
		{
			while (i = *(ptricmds++))
			{
				if (i < 0)
				{
					glBegin( GL_TRIANGLE_FAN );
					i = -i;
				}
				else
				{
					glBegin( GL_TRIANGLE_STRIP );
				}

				for( ; i > 0; i--, ptricmds += 4)
				{
					// FIX: put these in as integer coords, not floats
					glTexCoord2f(ptricmds[2]*s, ptricmds[3]*t);
					
					lv = g_pvlightvalues[ptricmds[1]];
					glColor4f( lv[0], lv[1], lv[2], 1.0 );

					av = g_pxformverts[ptricmds[0]];
					glVertex3f(av[0], av[1], av[2]);
				}
				glEnd();
			}	
		}
	}
}

static mstudiomodel_t *Studio_SetupModel( studiohdr_t *phdr, int bodypart )
{
	int index;

	if ( bodypart > phdr->numbodyparts )
	{
		Com_DPrintf( "Studio_SetupModel: no such bodypart %d\n", bodypart );
		bodypart = 0;
	}

	mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)( (byte *)phdr + phdr->bodypartindex ) + bodypart;

	index = m_bodynum / pbodypart->base;
	index = index % pbodypart->nummodels;

	return (mstudiomodel_t *)( (byte *)phdr + pbodypart->modelindex ) + index;
}

/*
=================
R_DrawStudioModel

=================
*/
void R_DrawStudioModel( entity_t *e )
{
	int i;
	mstudiomodel_t *pmodel;
	studiohdr_t *phdr;

	phdr = (studiohdr_t *)currentmodel->extradata;

	++g_smodels_total; // render data cache cookie

	g_pxformverts = &g_xformverts[0];
	g_pvlightvalues = &g_lightvalues[0];

	if ( phdr->numbodyparts == 0 )
		return;

	glPushMatrix();
//	e->angles[PITCH] = -e->angles[PITCH];	// sigh.
	R_RotateForEntity( e );
//	e->angles[PITCH] = -e->angles[PITCH];	// sigh.

	// draw it

	glShadeModel( GL_SMOOTH );
	GL_TexEnv( GL_MODULATE );

	Studio_SetUpBones( phdr );

	Studio_SetupLighting( phdr );

	// whatever
	for ( i = 0; i < phdr->numbodyparts; ++i )
	{
		pmodel = Studio_SetupModel( phdr, i );
		Studio_DrawPoints( phdr, phdr, pmodel, e->origin );
	}

	// Debug
	Studio_AdvanceFrame( phdr, r_newrefdef.frametime );

	GL_TexEnv( GL_REPLACE );
	glShadeModel( GL_FLAT );

	glPopMatrix();
}

#else

void R_DrawStudioModel( entity_t *e )
{

}

#endif

/*
===================================================================================================

	Static Mesh Files

===================================================================================================
*/

// TODO: there needs to be a constant that defines the max boundaries of the map, try to match Q3 eventually
// pretend that BIG_FLOAT_VALUE is the max distance you can travel from the origin
#define BIG_FLOAT_VALUE 131072.0f

#define MAX_LIGHTS	4

struct renderLight_t
{
	vec3_t position;
	vec3_t color;
	float intensity;
};

struct checkLight_t
{
	float distance;
	int index;
};

static DirectX::XMFLOAT4X4 R_CreateViewMatrix( const vec3 &origin, const vec3 &forward, const vec3 &up )
{
	using namespace DirectX;

	XMVECTOR zaxis = XMVector3Normalize( XMVectorNegate( XMLoadFloat3( (const XMFLOAT3 *)&forward ) ) );
	XMVECTOR yaxis = XMLoadFloat3( (const XMFLOAT3 *)&up );
	XMVECTOR xaxis = XMVector3Normalize( XMVector3Cross( yaxis, zaxis ) );
	yaxis = XMVector3Cross( zaxis, xaxis );

	XMFLOAT4X4 r;
	XMStoreFloat3( reinterpret_cast<XMFLOAT3 *>( &r._11 ), xaxis );
	XMStoreFloat3( reinterpret_cast<XMFLOAT3 *>( &r._21 ), yaxis );
	XMStoreFloat3( reinterpret_cast<XMFLOAT3 *>( &r._31 ), zaxis );
	r._14 = 0.0f;
	r._24 = 0.0f;
	r._34 = 0.0f;
	r._41 = origin.x;
	r._42 = origin.y;
	r._43 = origin.z;
	r._44 = 1.f;
	return r;
}

#undef countof

#if 0
#define GLM_FORCE_EXPLICIT_CTOR
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#endif

void R_DrawStaticMeshFile( entity_t *e )
{
	using namespace DirectX;

	mSMF_t *memSMF = (mSMF_t *)e->model->extradata;

	// Matrices

	float rotate = anglemod( r_newrefdef.time * 100.0f );

	XMMATRIX modelMatrix = XMMatrixMultiply(
		XMMatrixRotationRollPitchYaw( DEG2RAD( e->angles[PITCH] + rotate ), DEG2RAD( e->angles[ROLL] + rotate ), DEG2RAD( e->angles[YAW] + rotate ) ),
		//XMMatrixRotationRollPitchYaw( 0.0f, 0.0f, 0.0f ),
		XMMatrixTranslation( e->origin[0], e->origin[1], e->origin[2] )
	);

	XMFLOAT4X4A modelMatrixStore;
	XMStoreFloat4x4A( &modelMatrixStore, modelMatrix );

	float view[16];
	float proj[16];

	glGetFloatv( GL_MODELVIEW_MATRIX, view );
	glGetFloatv( GL_PROJECTION_MATRIX, proj );

	// Ambient colour

	vec3_t ambientColor;

	R_LightPoint( e->origin, ambientColor );

	// Lights

	// build a list of all lights in our PVS

	std::vector<staticLight_t> lightsInPVS;
	lightsInPVS.reserve( 32 );

	for ( int i = 0; i < mod_numStaticLights; ++i )
	{
		// hack into the server code, this doesn't call any server functions so we're safe, it's just a wrapper
		extern qboolean PF_inPVS( vec3_t p1, vec3_t p2 );

		if ( PF_inPVS( e->origin, mod_staticLights[i].origin ) )
		{
			lightsInPVS.push_back( mod_staticLights[i] );
		}
	}

	renderLight_t finalLights[MAX_LIGHTS]{};

	if ( lightsInPVS.size() != 0 )
	{
		int skipIndices[MAX_LIGHTS]{};

		// for all the lights in our PVS, find the four values that have the shortest distance to the origin, do this MAX_LIGHTS times
		for ( int iter1 = 0; iter1 < MAX_LIGHTS; ++iter1 )
		{
			float smallestDistance = BIG_FLOAT_VALUE;
			int smallestIndex = 0;

			for ( int iter2 = 0; iter2 < (int)lightsInPVS.size(); ++iter2 )
			{
				float distance = VectorDistance( e->origin, lightsInPVS[iter2].origin );
				if ( distance < smallestDistance )
				{
					if ( iter1 > 0 && skipIndices[iter1 - 1] == iter2 )
					{
						// skip, already got it
						continue;
					}
					smallestDistance = distance;
					smallestIndex = iter2;
				}
			}

			skipIndices[iter1] = smallestIndex;

			renderLight_t &finalLight = finalLights[iter1];
			staticLight_t &staticLight = lightsInPVS[smallestIndex];

			VectorCopy( staticLight.origin, finalLight.position );
			VectorCopy( staticLight.color, finalLight.color );
			finalLight.intensity = static_cast<float>( staticLight.intensity ) * 0.5; // compensate
		}
	}

	glUseProgram( glProgs.smfMeshProg );

	glUniformMatrix4fv( 3, 1, GL_FALSE, (const GLfloat *)&modelMatrixStore );
	glUniformMatrix4fv( 4, 1, GL_FALSE, (const GLfloat *)&view );
	glUniformMatrix4fv( 5, 1, GL_FALSE, (const GLfloat *)&proj );

	glUniform3fv( 6, 1, ambientColor );
	glUniform3fv( 7, 1, r_newrefdef.vieworg );

	constexpr int startIndex = 8;
	constexpr int elementsInRenderLight = 3;

	for ( int iter1 = 0, iter2 = 0; iter1 < MAX_LIGHTS; ++iter1, iter2 += elementsInRenderLight )
	{
		glUniform3fv( startIndex + iter2 + 0, 1, finalLights[iter1].position );
		glUniform3fv( startIndex + iter2 + 1, 1, finalLights[iter1].color );
		glUniform1f( startIndex + iter2 + 2, finalLights[iter1].intensity );
	}

	constexpr int indexAfterLights = startIndex + elementsInRenderLight * MAX_LIGHTS;

	glUniform1i( indexAfterLights + 0, 0 ); // diffuse
	glUniform1i( indexAfterLights + 1, 1 ); // specular
	glUniform1i( indexAfterLights + 2, 2 ); // emission

	glBindVertexArray( memSMF->vao );
	glBindBuffer( GL_ARRAY_BUFFER, memSMF->vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, memSMF->ebo );

	// HACK: this should go away when bsp rendering is nuked
	glActiveTexture( GL_TEXTURE0 );
	memSMF->material->Bind();
	glActiveTexture( GL_TEXTURE1 );
	memSMF->material->BindSpec();
	glActiveTexture( GL_TEXTURE2 );
	memSMF->material->BindEmit();

	glCullFace( GL_BACK );

	glDrawElements( GL_TRIANGLES, memSMF->numIndices, GL_UNSIGNED_SHORT, (void *)( 0 ) );

	glCullFace( GL_FRONT );

	// HACK: this should go away when bsp rendering is nuked
	GLenum hackTmu = glState.currenttmu ? GL_TEXTURE1 : GL_TEXTURE0;
	glActiveTexture( hackTmu );

	glUseProgram( 0 );
}
