// gl_main.c

#include "gl_local.h"

#include <vector>

model_t		*r_worldmodel;

double		gldepthmin, gldepthmax;

entity_t	*currententity;
model_t		*currentmodel;

cplane_t	frustum[4];

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

int			c_brush_polys, c_alias_polys;

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

// end vars

/*
========================
R_RotateForEntity
========================
*/
void R_RotateForEntity( entity_t *e )
{
	glTranslatef( e->origin[0], e->origin[1], e->origin[2] );

	glRotatef( e->angles[1], 0, 0, 1 );
	glRotatef( -e->angles[0], 0, 1, 0 );
	glRotatef( -e->angles[2], 1, 0, 0 );
}

//=================================================================================================

/*
===================================================================================================

	Model rendering

===================================================================================================
*/

/*
========================
R_DrawSpriteModel
========================
*/
static void R_DrawSpriteModel( entity_t *e )
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

/*
========================
R_DrawBeam
========================
*/
#define NUM_BEAM_SEGS 6

void R_DrawBeam( entity_t *e )
{
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

/*
========================
R_DrawNullModel

Draws a little thingy in place of a model, cool!
========================
*/
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

/*
========================
R_DrawEntitiesOnList

This actually performs rendering
========================
*/
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
				Com_Errorf("Bad modeltype" );
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
				Com_Errorf("Bad modeltype" );
				break;
			}
		}
	}
	glDepthMask( 1 );		// back to writing

}

/*
===================================================================================================

	Particle rendering

	Particles aren't models, so they get drawn in a huge clump after the world and entities.
	Eventually we want these to become textured, and be read from a script or something
	because right now they're kinda boring

===================================================================================================
*/

// 16 bytes
struct partPoint_t
{
	float x, y, z;		// 12 bytes
	byte r, g, b, a;	// 4 bytes
};

static GLuint s_partVAO;
static GLuint s_partVBO;

std::vector<partPoint_t> s_partVector;

/*
========================
Particles_Init
========================
*/
void Particles_Init()
{
	glGenVertexArrays( 1, &s_partVAO );
	glGenBuffers( 1, &s_partVBO );

	glBindVertexArray( s_partVAO );
	glBindBuffer( GL_ARRAY_BUFFER, s_partVBO );

	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( partPoint_t ), (void *)( 0 ) );
	glVertexAttribPointer( 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( partPoint_t ), (void *)( 3 * sizeof( GLfloat ) ) );

	// this sucks a little, we'd rather have our own class for this
	s_partVector.reserve( MAX_PARTICLES / 2 );
}

/*
========================
Particles_Shutdown
========================
*/
void Particles_Shutdown()
{
	glDeleteBuffers( 1, &s_partVBO );
	glDeleteVertexArrays( 1, &s_partVAO );
}

/*
========================
R_DrawParticles
========================
*/
static void R_DrawParticles()
{
	if ( r_newrefdef.num_particles <= 0 ) {
		return;
	}

	int i;
	particle_t *p;

	// ensure we have enough room
	s_partVector.reserve( r_newrefdef.num_particles );

	for ( i = 0, p = r_newrefdef.particles; i < r_newrefdef.num_particles; i++, p++ )
	{
		partPoint_t &point = s_partVector.emplace_back();

		point.x = p->origin[0];
		point.y = p->origin[1];
		point.z = p->origin[2];

		point.r = ((byte *)&d_8to24table[p->color])[0];
		point.g = ((byte *)&d_8to24table[p->color])[1];
		point.b = ((byte *)&d_8to24table[p->color])[2];
		point.a = static_cast<byte>( p->alpha * 255.0f );
	}

	// set up the render state
//	DirectX::XMMATRIX perspMatrix = DirectX::XMMatrixPerspectiveFovRH( r_newrefdef.fov_y, vid.width / vid.height, 4.0f, 4096.0f );
//	perspMatrix = DirectX::XMMatrixTranspose( perspMatrix );

	float proj[16];
	float view[16];

	glGetFloatv( GL_PROJECTION_MATRIX, proj );
	glGetFloatv( GL_MODELVIEW_MATRIX, view );

	glUseProgram( glProgs.particleProg );
	glUniformMatrix4fv( 2, 1, GL_FALSE, proj );
	glUniformMatrix4fv( 3, 1, GL_FALSE, view );

	glBindVertexArray( s_partVAO );
	glBindBuffer( GL_ARRAY_BUFFER, s_partVBO );

	glBufferData( GL_ARRAY_BUFFER, r_newrefdef.num_particles * sizeof( partPoint_t ), (void *)s_partVector.data(), GL_STREAM_DRAW );

	s_partVector.clear();

	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );

	glDrawArrays( GL_POINTS, 0, r_newrefdef.num_particles );

	glDisable( GL_BLEND );
	glDepthMask( GL_TRUE );
}

//=================================================================================================

/*
========================
R_SetFrustum
========================
*/
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
		frustum[i].signbits = SignbitsForPlane( frustum[i] );
	}
}

/*
========================
R_SetupFrame
========================
*/
static void R_SetupFrame()
{
	mleaf_t *leaf;

	r_framecount++;

	// build the transformation matrix for the given view angles
	VectorCopy( r_newrefdef.vieworg, r_origin );

	AngleVectors( r_newrefdef.viewangles, vpn, vright, vup );

	// current viewcluster
	if ( !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf( r_origin, r_worldmodel );
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if ( !leaf->contents )
		{
			// look down a bit
			vec3_t	temp;

			VectorCopy( r_origin, temp );
			temp[2] -= 16;
			leaf = Mod_PointInLeaf( temp, r_worldmodel );
			if ( !( leaf->contents & CONTENTS_SOLID ) && ( leaf->cluster != r_viewcluster2 ) ) {
				r_viewcluster2 = leaf->cluster;
			}
		}
		else
		{
			// look up a bit
			vec3_t	temp;

			VectorCopy( r_origin, temp );
			temp[2] += 16;
			leaf = Mod_PointInLeaf( temp, r_worldmodel );
			if ( !( leaf->contents & CONTENTS_SOLID ) && ( leaf->cluster != r_viewcluster2 ) ) {
				r_viewcluster2 = leaf->cluster;
			}
		}
	}

	c_brush_polys = 0;
	c_alias_polys = 0;

	// clear out the portion of the screen that the NOWORLDMODEL defines
	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
	{
		glEnable( GL_SCISSOR_TEST );
		glClearColor( 0.3f, 0.3f, 0.3f, 1.0f );
		glScissor( r_newrefdef.x, vid.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width, r_newrefdef.height );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glClearColor( DEFAULT_CLEARCOLOR );
		glDisable( GL_SCISSOR_TEST );
	}
}

/*
========================
MYgluPerspective
========================
*/
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

/*
========================
R_SetupGL

GET RID OF ME!!!
========================
*/
static void R_SetupGL()
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

#if 1
	glRotatef( -90, 1, 0, 0 );	    // put Z going up
	glRotatef( 90, 0, 0, 1 );	    // put Z going up
	glRotatef( -r_newrefdef.viewangles[2], 1, 0, 0 );
	glRotatef( -r_newrefdef.viewangles[0], 0, 1, 0 );
	glRotatef( -r_newrefdef.viewangles[1], 0, 0, 1 );
	glTranslatef( -r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2] );
#endif

	glGetFloatv( GL_MODELVIEW_MATRIX, r_world_matrix );

	//
	// set drawing parms
	//
	if ( r_cullfaces->GetBool() ) {
		glEnable( GL_CULL_FACE );
	} else {
		glDisable( GL_CULL_FACE );
	}

	if ( r_wireframe->IsModified() ) {
		r_wireframe->ClearModified();
		glPolygonMode( GL_FRONT_AND_BACK, r_wireframe->GetBool() ? GL_LINE : GL_FILL );
	}

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_TEXTURE_2D );
}

/*
========================
R_Clear

Performs a screen clear
========================
*/
static void R_Clear()
{
	if ( r_clear->value ) {
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	} else {
		glClear( GL_DEPTH_BUFFER_BIT );
	}

	gldepthmin = 0.0;
	gldepthmax = 1.0;
	glDepthFunc( GL_LEQUAL );
	glDepthRange( gldepthmin, gldepthmax );
}

/*
========================
R_RenderView

r_newrefdef must be set before the first call
========================
*/
static void R_RenderView( refdef_t *fd )
{
	if ( r_norefresh->value ) {
		return;
	}

	r_newrefdef = *fd;

	if ( !r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) ) {
		Com_Error( "R_RenderView: NULL worldmodel" );
	}

	if ( r_speeds->value )
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	R_PushDlights();

	if ( r_finish->value ) {
		glFinish();
	}

	R_SetupFrame();

	R_SetFrustum();

	// reset state
	R_SetupGL();

	// done here so we know if we're in water
	R_MarkLeaves();

	// world
	R_DrawWorld();

	// entities
	R_DrawEntitiesOnList();

	// ugly dlights approximation
	R_RenderDlights();

	// particles!
	R_DrawParticles();

	// world alpha surfaces
	R_DrawAlphaSurfaces();

	// screen overlay
	R_DrawScreenOverlay( r_newrefdef.blend );

	if ( r_speeds->value )
	{
		Com_Printf( "%4i wpoly %4i epoly %i tex %i lmaps\n",
			c_brush_polys,
			c_alias_polys,
			c_visible_textures,
			c_visible_lightmaps );
	}
}

/*
========================
R_SetLightLevel
========================
*/
static void R_SetLightLevel()
{
	vec3_t shadelight;

	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) {
		return;
	}

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

/*
========================
R_SetMode
========================
*/
static void R_SetMode()
{
	if ( r_mode->IsModified() || r_fullscreen->IsModified() )
	{
		r_mode->ClearModified();
		r_fullscreen->ClearModified();

		if ( GLimp_SetMode( vid.width, vid.height, r_mode->GetInt32(), r_fullscreen->GetBool() ) == true )
		{
			glState.prev_mode = r_mode->GetInt32();
		}
		else
		{
			Cvar_SetValue( "r_mode", glState.prev_mode );
			r_mode->ClearModified();
			Com_Printf( "R_SetMode: invalid mode\n" );

			// try setting it back to something safe
			if ( GLimp_SetMode( vid.width, vid.height, glState.prev_mode, false ) == false )
			{
				Com_FatalError( "R_SetMode: could not revert to safe mode" );
			}
		}
	}
}

/*
========================
R_BeginFrame

Public
========================
*/
void R_BeginFrame()
{
	GLimp_BeginFrame();

	// check if we need to set modes
	R_SetMode();

	R_Clear();
}

/*
========================
R_RenderFrame

Public
========================
*/
void R_RenderFrame( refdef_t *fd )
{
	R_RenderView( fd );
	R_SetLightLevel();
}

/*
========================
R_EndFrame

Public
========================
*/
void R_EndFrame()
{
	Draw_RenderBatches();

	GL_CheckErrors();

	GLimp_EndFrame();
}
