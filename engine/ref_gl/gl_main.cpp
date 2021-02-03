// gl_main.c

#include "gl_local.h"

viddef_t	vid;

model_t		*r_worldmodel;

double		gldepthmin, gldepthmax;

glconfig_t	gl_config;
glstate_t	gl_state;

entity_t	*currententity;
model_t		*currentmodel;

cplane_t	frustum[4];

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

int			c_brush_polys, c_alias_polys;

static vec4_t	v_blend;		// final blending color

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];

//
// screen size info
//
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lerpmodels;
cvar_t	*r_lefthand;

cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

cvar_t	*gl_nosubimage;
cvar_t	*gl_allow_software;

cvar_t	*gl_vertex_arrays;

cvar_t	*gl_particle_min_size;
cvar_t	*gl_particle_max_size;
cvar_t	*gl_particle_size;
cvar_t	*gl_particle_att_a;
cvar_t	*gl_particle_att_b;
cvar_t	*gl_particle_att_c;

cvar_t	*gl_ext_swapinterval;
cvar_t	*gl_ext_multitexture;
cvar_t	*gl_ext_pointparameters;
cvar_t	*gl_ext_compiled_vertex_array;

cvar_t	*gl_lightmap;
cvar_t	*gl_shadows;
cvar_t	*gl_mode;
cvar_t	*gl_dynamic;
cvar_t	*gl_modulate;
cvar_t	*gl_picmip;
cvar_t	*gl_skymip;
cvar_t	*gl_showtris;
cvar_t	*gl_finish;
cvar_t	*gl_clear;
cvar_t	*gl_cull;
cvar_t	*gl_polyblend;
cvar_t	*gl_flashblend;
cvar_t  *gl_overbright;
cvar_t	*gl_swapinterval;
cvar_t	*gl_lockpvs;

//-------------------------------------------------------------------------------------------------
// Returns true if the box is completely outside the frustom
//-------------------------------------------------------------------------------------------------
qboolean R_CullBox( vec3_t mins, vec3_t maxs )
{
	int i;

	if ( r_nocull->value )
		return false;

	for ( i = 0; i < 4; i++ )
		if ( BOX_ON_PLANE_SIDE( mins, maxs, &frustum[i] ) == 2 )
			return true;
	return false;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void R_RotateForEntity( entity_t *e )
{
	glTranslatef( e->origin[0], e->origin[1], e->origin[2] );

	glRotatef( e->angles[1], 0, 0, 1 );
	glRotatef( -e->angles[0], 0, 1, 0 );
	glRotatef( -e->angles[2], 1, 0, 0 );
}

//=================================================================================================

//-------------------------------------------------------------------------------------------------
// Draws a sprite, similar to the functions in gl_mesh
//-------------------------------------------------------------------------------------------------
void R_DrawSpriteModel( entity_t *e )
{
	float alpha = 1.0f;
	vec3_t point;
	dsprframe_t *frame;
	float *up, *right;
	dsprite_t *psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	psprite = (dsprite_t *)currentmodel->extradata;

#if 0
	if ( e->frame < 0 || e->frame >= psprite->numframes )
	{
		Com_Printf( "no such sprite frame %i\n", e->frame );
		e->frame = 0;
	}
#endif
	e->frame %= psprite->numframes;

	frame = &psprite->frames[e->frame];

#if 0
	if ( psprite->type == SPR_ORIENTED )
	{	// bullet marks on walls
		vec3_t		v_forward, v_right, v_up;

		AngleVectors( currententity->angles, v_forward, v_right, v_up );
		up = v_up;
		right = v_right;
	}
	else
#endif
	{	// normal sprite
		up = vup;
		right = vright;
	}

	if ( e->flags & RF_TRANSLUCENT )
		alpha = e->alpha;

	if ( alpha != 1.0f )
		glEnable( GL_BLEND );

	glColor4f( 1, 1, 1, alpha );

	currentmodel->skins[e->frame]->Bind();

	GL_TexEnv( GL_MODULATE );

	if ( alpha == 1.0f )
		glEnable( GL_ALPHA_TEST );
	else
		glDisable( GL_ALPHA_TEST );

	glBegin( GL_QUADS );

	glTexCoord2f( 0, 1 );
	VectorMA( e->origin, -frame->origin_y, up, point );
	VectorMA( point, -frame->origin_x, right, point );
	glVertex3fv( point );

	glTexCoord2f( 0, 0 );
	VectorMA( e->origin, frame->height - frame->origin_y, up, point );
	VectorMA( point, -frame->origin_x, right, point );
	glVertex3fv( point );

	glTexCoord2f( 1, 0 );
	VectorMA( e->origin, frame->height - frame->origin_y, up, point );
	VectorMA( point, frame->width - frame->origin_x, right, point );
	glVertex3fv( point );

	glTexCoord2f( 1, 1 );
	VectorMA( e->origin, -frame->origin_y, up, point );
	VectorMA( point, frame->width - frame->origin_x, right, point );
	glVertex3fv( point );

	glEnd();

	glDisable( GL_ALPHA_TEST );
	GL_TexEnv( GL_REPLACE );

	if ( alpha != 1.0f )
		glDisable( GL_BLEND );

	glColor4f( 1, 1, 1, 1 );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void R_DrawNullModel()
{
	vec3_t shadelight;
	int i;

	if ( currententity->flags & RF_FULLBRIGHT )
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0f;
	else
		R_LightPoint( currententity->origin, shadelight );

	glPushMatrix();
	R_RotateForEntity( currententity );

	glDisable( GL_TEXTURE_2D );
	glColor3fv( shadelight );

	glBegin( GL_TRIANGLE_FAN );
	glVertex3f( 0, 0, -16 );
	for ( i=0; i<=4; i++ )
		glVertex3f (16*cosf(i*M_PI_F/2), 16*sinf(i*M_PI_F/2), 0);
	glEnd();

	glBegin( GL_TRIANGLE_FAN );
	glVertex3f( 0, 0, 16 );
	for ( i=4; i>=0; i-- )
		glVertex3f( 16*cosf(i*M_PI_F/2), 16*sinf(i*M_PI_F/2), 0 );
	glEnd();

	glColor3f( 1,1,1 );
	glPopMatrix();
	glEnable( GL_TEXTURE_2D );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void R_DrawEntitiesOnList()
{
	int i;

	if ( !r_drawentities->value )
		return;

	// draw non-transparent first
	for ( i = 0; i < r_newrefdef.num_entities; i++ )
	{
		currententity = &r_newrefdef.entities[i];
		if ( currententity->flags & RF_TRANSLUCENT )
			continue;	// solid

		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;

			if ( !currentmodel )
			{
				R_DrawNullModel();
				continue;
			}
			switch ( currentmodel->type )
			{
			case mod_alias:
				R_DrawAliasModel( currententity );
				break;
			case mod_studio:
				R_DrawStudioModel( currententity );
				break;
			case mod_brush:
				R_DrawBrushModel( currententity );
				break;
			case mod_sprite:
				R_DrawSpriteModel( currententity );
				break;
			default:
				Com_Error( ERR_DROP, "Bad modeltype" );
				break;
			}
		}
	}

	// draw transparent entities
	// we could sort these if it ever becomes a problem...
	glDepthMask( 0 );		// no z writes
	for ( i = 0; i < r_newrefdef.num_entities; i++ )
	{
		currententity = &r_newrefdef.entities[i];
		if ( !( currententity->flags & RF_TRANSLUCENT ) )
			continue;	// solid

		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;

			if ( !currentmodel )
			{
				R_DrawNullModel();
				continue;
			}
			switch ( currentmodel->type )
			{
			case mod_alias:
				R_DrawAliasModel( currententity );
				break;
			case mod_studio:
				R_DrawStudioModel( currententity );
				break;
			case mod_brush:
				R_DrawBrushModel( currententity );
				break;
			case mod_sprite:
				R_DrawSpriteModel( currententity );
				break;
			default:
				Com_Error( ERR_DROP, "Bad modeltype" );
				break;
			}
		}
	}
	glDepthMask( 1 );		// back to writing

}

//-------------------------------------------------------------------------------------------------
// Never called if point parameters are enabled
//-------------------------------------------------------------------------------------------------
static void GL_DrawParticles( int num_particles, const particle_t *particles, const unsigned *colortable )
{
	const particle_t *p;
	int				i;
	vec3_t			up, right;
	float			scale;
	byte			color[4];

	mat_particletexture->Bind();

	glDepthMask( GL_FALSE );		// no z buffering
	glEnable( GL_BLEND );
	GL_TexEnv( GL_MODULATE );
	glBegin( GL_TRIANGLES );

	VectorScale( vup, 1.5f, up );
	VectorScale( vright, 1.5f, right );

	for ( p = particles, i = 0; i < num_particles; i++, p++ )
	{
		// hack a scale up to keep particles from disapearing
		scale = ( p->origin[0] - r_origin[0] ) * vpn[0] +
				( p->origin[1] - r_origin[1] ) * vpn[1] +
				( p->origin[2] - r_origin[2] ) * vpn[2];

		if ( scale < 20 )
			scale = 1;
		else
			scale = 1 + scale * 0.004f;

		*(int *)color = colortable[p->color];
		color[3] = p->alpha * 255;

		glColor4ubv( color );

		glTexCoord2f( 0.0625f, 0.0625f );
		glVertex3fv( p->origin );

		glTexCoord2f( 1.0625f, 0.0625f );
		glVertex3f( p->origin[0] + up[0] * scale,
					p->origin[1] + up[1] * scale,
					p->origin[2] + up[2] * scale );

		glTexCoord2f( 0.0625f, 1.0625f );
		glVertex3f( p->origin[0] + right[0] * scale,
					p->origin[1] + right[1] * scale,
					p->origin[2] + right[2] * scale );
	}

	glEnd();
	glDisable( GL_BLEND );
	glColor4f( 1, 1, 1, 1 );
	glDepthMask( 1 );		// back to normal Z buffering
	GL_TexEnv( GL_REPLACE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void R_DrawParticles()
{
	if ( GLEW_EXT_point_parameters && gl_ext_pointparameters->value )
	{
		int i;
		unsigned char color[4];
		const particle_t *p;

		glDepthMask( GL_FALSE );
		glEnable( GL_BLEND );
		glDisable( GL_TEXTURE_2D );

		glPointSize( gl_particle_size->value );

		glBegin( GL_POINTS );
		for ( i = 0, p = r_newrefdef.particles; i < r_newrefdef.num_particles; i++, p++ )
		{
			*(int *)color = d_8to24table[p->color];
			color[3] = p->alpha * 255;

			glColor4ubv( color );

			glVertex3fv( p->origin );
		}
		glEnd();

		glDisable( GL_BLEND );
		glColor4f( 1.0F, 1.0F, 1.0F, 1.0F );
		glDepthMask( GL_TRUE );
		glEnable( GL_TEXTURE_2D );
	}
	else
	{
		GL_DrawParticles( r_newrefdef.num_particles, r_newrefdef.particles, d_8to24table );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void R_PolyBlend()
{
	if ( !gl_polyblend->value )
		return;
	if ( !v_blend[3] )
		return;

	glDisable( GL_ALPHA_TEST );
	glEnable( GL_BLEND );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_TEXTURE_2D );

	glLoadIdentity();

	// FIXME: get rid of these
	glRotatef( -90, 1, 0, 0 );	    // put Z going up
	glRotatef( 90, 0, 0, 1 );	    // put Z going up

	glColor4fv( v_blend );

	glBegin( GL_QUADS );

	glVertex3f( 10, 100, 100 );
	glVertex3f( 10, -100, 100 );
	glVertex3f( 10, -100, -100 );
	glVertex3f( 10, 100, -100 );
	glEnd();

	glDisable( GL_BLEND );
	glEnable( GL_TEXTURE_2D );
	glEnable( GL_ALPHA_TEST );

	glColor4f( 1, 1, 1, 1 );
}

//=================================================================================================

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int SignbitsForPlane( cplane_t *out )
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for ( j = 0; j < 3; j++ )
	{
		if ( out->normal[j] < 0 )
			bits |= 1 << j;
	}
	return bits;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void R_SetFrustum()
{
	int i;

#if 0
	/*
	** this code is wrong, since it presume a 90 degree FOV both in the
	** horizontal and vertical plane
	*/
	// front side is visible
	VectorAdd( vpn, vright, frustum[0].normal );
	VectorSubtract( vpn, vright, frustum[1].normal );
	VectorAdd( vpn, vup, frustum[2].normal );
	VectorSubtract( vpn, vup, frustum[3].normal );

	// we theoretically don't need to normalize these vectors, but I do it
	// anyway so that debugging is a little easier
	VectorNormalize( frustum[0].normal );
	VectorNormalize( frustum[1].normal );
	VectorNormalize( frustum[2].normal );
	VectorNormalize( frustum[3].normal );
#else
	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector( frustum[0].normal, vup, vpn, -( 90 - r_newrefdef.fov_x / 2 ) );
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector( frustum[1].normal, vup, vpn, 90 - r_newrefdef.fov_x / 2 );
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector( frustum[2].normal, vright, vpn, 90 - r_newrefdef.fov_y / 2 );
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_newrefdef.fov_y / 2 ) );
#endif

	for ( i = 0; i < 4; i++ )
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct( r_origin, frustum[i].normal );
		frustum[i].signbits = SignbitsForPlane( &frustum[i] );
	}
}

//=================================================================================================

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void R_SetupFrame (void)
{
	int i;
	mleaf_t	*leaf;

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_newrefdef.vieworg, r_origin);

	AngleVectors (r_newrefdef.viewangles, vpn, vright, vup);

// current viewcluster
	if ( !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf (r_origin, r_worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
	}

	for (i=0 ; i<4 ; i++)
		v_blend[i] = r_newrefdef.blend[i];

	c_brush_polys = 0;
	c_alias_polys = 0;

	// clear out the portion of the screen that the NOWORLDMODEL defines
	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
	{
		glEnable( GL_SCISSOR_TEST );
		glClearColor( 0.3, 0.3, 0.3, 1 );
		glScissor( r_newrefdef.x, vid.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width, r_newrefdef.height );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glClearColor( DEFAULT_CLEARCOLOR );
		glDisable( GL_SCISSOR_TEST );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	xmin += -(2.0 * 0.0) / zNear;
	xmax += -(2.0 * 0.0) / zNear;

	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

//-------------------------------------------------------------------------------------------------
// Called every frame
//-------------------------------------------------------------------------------------------------
void R_SetupGL()
{
	//
	// set up projection matrix
	//
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	MYgluPerspective( r_newrefdef.fov_y, (GLdouble)r_newrefdef.width / (GLdouble)r_newrefdef.height, 4.0, 4096.0 );

	glCullFace( GL_FRONT );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glRotatef( -90, 1, 0, 0 );	    // put Z going up
	glRotatef( 90, 0, 0, 1 );	    // put Z going up
	glRotatef( -r_newrefdef.viewangles[2], 1, 0, 0 );
	glRotatef( -r_newrefdef.viewangles[0], 0, 1, 0 );
	glRotatef( -r_newrefdef.viewangles[1], 0, 0, 1 );
	glTranslatef( -r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2] );

	glGetFloatv( GL_MODELVIEW_MATRIX, r_world_matrix );

	//
	// set drawing parms
	//
	if ( gl_cull->value )
		glEnable( GL_CULL_FACE );
	else
		glDisable( GL_CULL_FACE );

	glDisable( GL_BLEND );
	glDisable( GL_ALPHA_TEST );
	glEnable( GL_DEPTH_TEST );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void R_Clear()
{
	if ( gl_clear->value )
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	else
		glClear( GL_DEPTH_BUFFER_BIT );
	gldepthmin = 0.0;
	gldepthmax = 1.0;
	glDepthFunc( GL_LEQUAL );

	glDepthRange( gldepthmin, gldepthmax );
}

//-------------------------------------------------------------------------------------------------
// r_newrefdef must be set before the first call
//-------------------------------------------------------------------------------------------------
void R_RenderView( refdef_t *fd )
{
	if ( r_norefresh->value )
		return;

	r_newrefdef = *fd;

	if ( !r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		Com_Error( ERR_DROP, "R_RenderView: NULL worldmodel" );

	if ( r_speeds->value )
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	R_PushDlights();

	if ( gl_finish->value )
		glFinish();

	R_SetupFrame();

	R_SetFrustum();

	R_SetupGL();

	R_MarkLeaves(); // done here so we know if we're in water

	R_DrawWorld();

	R_DrawEntitiesOnList();

	R_RenderDlights();

	R_DrawParticles();

	R_DrawAlphaSurfaces();

	R_PolyBlend();

	if ( r_speeds->value )
	{
		Com_Printf( "%4i wpoly %4i epoly %i tex %i lmaps\n",
			c_brush_polys,
			c_alias_polys,
			c_visible_textures,
			c_visible_lightmaps );
	}
}

//-------------------------------------------------------------------------------------------------
// Sets some OpenGL state variables
// Called once by R_Init
//-------------------------------------------------------------------------------------------------
static void GL_SetDefaultState()
{
	glClearColor( DEFAULT_CLEARCOLOR );
	glCullFace( GL_FRONT );
	glEnable( GL_TEXTURE_2D );

	glEnable( GL_ALPHA_TEST );
	glAlphaFunc( GL_GREATER, 0.666f );

	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glDisable( GL_BLEND );

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	glShadeModel( GL_FLAT );

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	GL_TexEnv( GL_REPLACE );

	if ( GLEW_EXT_point_parameters && gl_ext_pointparameters->value )
	{
		float attenuations[3];

		attenuations[0] = gl_particle_att_a->value;
		attenuations[1] = gl_particle_att_b->value;
		attenuations[2] = gl_particle_att_c->value;

		glEnable( GL_POINT_SMOOTH );
		glPointParameterfEXT( GL_POINT_SIZE_MIN_EXT, gl_particle_min_size->value );
		glPointParameterfEXT( GL_POINT_SIZE_MAX_EXT, gl_particle_max_size->value );
		glPointParameterfvEXT( GL_DISTANCE_ATTENUATION_EXT, attenuations );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void R_SetGL2D()
{
	// set 2D virtual screen size
	glViewport( 0, 0, vid.width, vid.height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( 0, vid.width, vid.height, 0, -99999, 99999 );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glDisable( GL_BLEND );
	glEnable( GL_ALPHA_TEST );
	glColor4f( 1, 1, 1, 1 );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void R_SetLightLevel()
{
	vec3_t shadelight;

	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
		return;

	// save off light value for server to look at (BIG HACK!)

	R_LightPoint( r_newrefdef.vieworg, shadelight );

	// pick the greatest component, which should be the same
	// as the mono value returned by software
	if ( shadelight[0] > shadelight[1] )
	{
		if ( shadelight[0] > shadelight[2] )
			r_lightlevel->value = 150 * shadelight[0];
		else
			r_lightlevel->value = 150 * shadelight[2];
	}
	else
	{
		if ( shadelight[1] > shadelight[2] )
			r_lightlevel->value = 150 * shadelight[1];
		else
			r_lightlevel->value = 150 * shadelight[2];
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void R_Register()
{
	r_lefthand = Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	r_norefresh = Cvar_Get ("r_norefresh", "0", 0);
	r_fullbright = Cvar_Get ("r_fullbright", "0", 0);
	r_drawentities = Cvar_Get ("r_drawentities", "1", 0);
	r_drawworld = Cvar_Get ("r_drawworld", "1", 0);
	r_novis = Cvar_Get ("r_novis", "0", 0);
	r_nocull = Cvar_Get ("r_nocull", "0", 0);
	r_lerpmodels = Cvar_Get ("r_lerpmodels", "1", 0);
	r_speeds = Cvar_Get ("r_speeds", "0", 0);

	r_lightlevel = Cvar_Get ("r_lightlevel", "0", 0);

	gl_nosubimage = Cvar_Get( "gl_nosubimage", "0", 0 );
	gl_allow_software = Cvar_Get( "gl_allow_software", "0", 0 );

	gl_particle_min_size = Cvar_Get( "gl_particle_min_size", "2", CVAR_ARCHIVE );
	gl_particle_max_size = Cvar_Get( "gl_particle_max_size", "40", CVAR_ARCHIVE );
	gl_particle_size = Cvar_Get( "gl_particle_size", "40", CVAR_ARCHIVE );
	gl_particle_att_a = Cvar_Get( "gl_particle_att_a", "0.01", CVAR_ARCHIVE );
	gl_particle_att_b = Cvar_Get( "gl_particle_att_b", "0.0", CVAR_ARCHIVE );
	gl_particle_att_c = Cvar_Get( "gl_particle_att_c", "0.01", CVAR_ARCHIVE );

	gl_modulate = Cvar_Get ("gl_modulate", "1", CVAR_ARCHIVE );
	gl_mode = Cvar_Get( "gl_mode", "0", CVAR_ARCHIVE );
	gl_lightmap = Cvar_Get ("gl_lightmap", "0", 0);
	gl_shadows = Cvar_Get ("gl_shadows", "0", CVAR_ARCHIVE );
	gl_dynamic = Cvar_Get ("gl_dynamic", "1", 0);
	gl_picmip = Cvar_Get ("gl_picmip", "0", 0);
	gl_skymip = Cvar_Get ("gl_skymip", "0", 0);
	gl_showtris = Cvar_Get ("gl_showtris", "0", 0);
	gl_finish = Cvar_Get ("gl_finish", "0", CVAR_ARCHIVE);
	gl_clear = Cvar_Get ("gl_clear", "1", 0);
	gl_cull = Cvar_Get ("gl_cull", "1", 0);
	gl_polyblend = Cvar_Get ("gl_polyblend", "1", 0);
	gl_flashblend = Cvar_Get ("gl_flashblend", "0", 0);
	gl_lockpvs = Cvar_Get( "gl_lockpvs", "0", 0 );

	gl_vertex_arrays = Cvar_Get( "gl_vertex_arrays", "0", CVAR_ARCHIVE );

	gl_ext_swapinterval = Cvar_Get( "gl_ext_swapinterval", "1", CVAR_ARCHIVE );
	gl_ext_multitexture = Cvar_Get( "gl_ext_multitexture", "1", CVAR_ARCHIVE );
	gl_ext_pointparameters = Cvar_Get( "gl_ext_pointparameters", "1", CVAR_ARCHIVE );
	gl_ext_compiled_vertex_array = Cvar_Get( "gl_ext_compiled_vertex_array", "1", CVAR_ARCHIVE );

	gl_swapinterval = Cvar_Get( "gl_swapinterval", "0", CVAR_ARCHIVE );

	gl_overbright = Cvar_Get( "gl_overbright", "1", 0 );

	Cmd_AddCommand( "imagelist", GL_ImageList_f );
	Cmd_AddCommand( "screenshot", GL_ScreenShot_f );
	Cmd_AddCommand( "modellist", Mod_Modellist_f );
	Cmd_AddCommand( "gl_strings", GL_Strings_f );

	Cmd_AddCommand( "extractwad", GL_ExtractWad_f );
	Cmd_AddCommand( "upgradewals", GL_UpgradeWals_f );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void R_Unregister()
{
	Cmd_RemoveCommand( "imagelist" );
	Cmd_RemoveCommand( "screenshot" );
	Cmd_RemoveCommand( "modellist" );
	Cmd_RemoveCommand( "gl_strings" );

	Cmd_RemoveCommand( "extractwad" );
	Cmd_RemoveCommand( "upgradewals" );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static bool R_SetMode()
{
	rserr_t err;

	if ( gl_mode->modified || vid_fullscreen->modified )
	{
		gl_mode->modified = false;
		vid_fullscreen->modified = false;

		if ( ( err = GLimp_SetMode( &vid.width, &vid.height, gl_mode->value, (bool)vid_fullscreen->value ) ) == rserr_ok )
		{
			gl_state.prev_mode = gl_mode->value;
		}
		else
		{
			if ( err == rserr_invalid_fullscreen )
			{
				Cvar_SetValue( "vid_fullscreen", 0 );
				vid_fullscreen->modified = false;
				Com_Printf( "ref_gl::R_SetMode() - fullscreen unavailable in this mode\n" );
				if ( ( err = GLimp_SetMode( &vid.width, &vid.height, gl_mode->value, false ) ) == rserr_ok )
					return true;
			}
			else if ( err == rserr_invalid_mode )
			{
				Cvar_SetValue( "gl_mode", gl_state.prev_mode );
				gl_mode->modified = false;
				Com_Printf( "ref_gl::R_SetMode() - invalid mode\n" );
			}

			// try setting it back to something safe
			if ( ( err = GLimp_SetMode( &vid.width, &vid.height, gl_state.prev_mode, false ) ) != rserr_ok )
			{
				Com_Printf( "ref_gl::R_SetMode() - could not revert to safe mode\n" );
				return false;
			}
		}
	}

	return true;
}

static void GL_CheckErrors(void)
{
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		Com_Printf("glGetError() = 0x%x\n", err);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
bool R_Init( void *hinstance, void *hWnd )
{
	Com_Printf( "ref_gl version: " REF_VERSION "\n" );

	extern float r_turbsin[256];
	for ( int j = 0; j < 256; j++ )
	{
		r_turbsin[j] *= 0.5f;
	}

	R_Register();

	// initialize OS-specific parts of OpenGL
	if ( !GLimp_Init( hinstance, hWnd ) )
	{
		return false;
	}

	// set our "safe" modes
	gl_state.prev_mode = 3;

	/*
	** get our various GL strings
	*/
	gl_config.vendor_string = (const char *)glGetString( GL_VENDOR );
	Com_Printf( "GL_VENDOR: %s\n", gl_config.vendor_string );
	gl_config.renderer_string = (const char *)glGetString( GL_RENDERER );
	Com_Printf( "GL_RENDERER: %s\n", gl_config.renderer_string );
	gl_config.version_string = (const char *)glGetString( GL_VERSION );
	Com_Printf( "GL_VERSION: %s\n", gl_config.version_string );
	gl_config.extensions_string = (const char *)glGetString( GL_EXTENSIONS );
	Com_Printf( "GL_EXTENSIONS: %s\n", gl_config.extensions_string );

	Cvar_Set( "scr_drawall", "0" );

	GL_SetDefaultState();

	GL_InitImages();
	//GLimp_SetGamma( g_gammatable, g_gammatable, g_gammatable );

	Mod_Init();
	Draw_InitLocal();

	// Check for errors
	GL_CheckErrors();

	return true;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void R_Shutdown()
{
	Mod_FreeAll();

	//GLimp_RestoreGamma();
	GL_ShutdownImages();

	/*
	** shut down OS specific OpenGL stuff like contexts, etc.
	*/
	GLimp_Shutdown();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void R_BeginFrame()
{
	GLimp_BeginFrame();

	// Check if we need to set modes
	R_SetMode();

	/*
	** go into 2D mode
	*/
	R_SetGL2D();

	//
	// clear screen if desired
	//
	R_Clear();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void R_RenderFrame( refdef_t *fd )
{
	R_RenderView( fd );
	R_SetLightLevel();
	R_SetGL2D();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void R_EndFrame()
{
	GL_CheckErrors();

	GLimp_EndFrame();
}

//=================================================================================================

//-------------------------------------------------------------------------------------------------
// Sets the palette used by draw_stretchraw
//-------------------------------------------------------------------------------------------------
unsigned r_rawpalette[256];

void R_SetRawPalette ( const unsigned char *palette )
{
	// We don't use the raw palette for anything but cinematics, so ignore NULL calls
	if ( !palette )
		return;

	int i;
	byte *rp = (byte *)r_rawpalette;

	for ( i = 0; i < 256; i++ )
	{
		rp[i*4+0] = palette[i*3+0];
		rp[i*4+1] = palette[i*3+1];
		rp[i*4+2] = palette[i*3+2];
		rp[i*4+3] = 0xff;
	}

	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT );
	glClearColor( DEFAULT_CLEARCOLOR );
}

//-------------------------------------------------------------------------------------------------
// fizzle
//-------------------------------------------------------------------------------------------------
void R_DrawBeam( entity_t *e )
{
#define NUM_BEAM_SEGS 6

	int	i;
	float r, g, b;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );
	VectorScale( perpvec, e->frame / 2, perpvec );

	for ( i = 0; i < 6; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0f/NUM_BEAM_SEGS)*i );
		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	glDisable( GL_TEXTURE_2D );
	glEnable( GL_BLEND );
	glDepthMask( GL_FALSE );

	r = ( d_8to24table[e->skinnum & 0xFF] ) & 0xFF;
	g = ( d_8to24table[e->skinnum & 0xFF] >> 8 ) & 0xFF;
	b = ( d_8to24table[e->skinnum & 0xFF] >> 16 ) & 0xFF;

	r *= 1/255.0F;
	g *= 1/255.0F;
	b *= 1/255.0F;

	glColor4f( r, g, b, e->alpha );

	glBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		glVertex3fv( start_points[i] );
		glVertex3fv( end_points[i] );
		glVertex3fv( start_points[(i+1)%NUM_BEAM_SEGS] );
		glVertex3fv( end_points[(i+1)%NUM_BEAM_SEGS] );
	}
	glEnd();

	glEnable( GL_TEXTURE_2D );
	glDisable( GL_BLEND );
	glDepthMask( GL_TRUE );
}
