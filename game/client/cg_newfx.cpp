/*
===============================================================================

	MORE entity effects parsing and management

	Quake 2 expansion pack effects (PGM)

===============================================================================
*/

#include "cg_local.h"

extern void MakeNormalVectors (vec3_t forward, vec3_t right, vec3_t up);

/*
===================
vectoangles2

This is duplicated in the game DLL, but I need it here.
===================
*/
static void vectoangles2( vec3_t value1, vec3_t angles )
{
	float	forward;
	float	yaw, pitch;

	if ( value1[1] == 0 && value1[0] == 0 )
	{
		yaw = 0;
		if ( value1[2] > 0 )
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
	// PMM - fixed to correct for pitch of 0
		if ( value1[0] )
			yaw = RAD2DEG( atan2f( value1[1], value1[0] ) );
		else if ( value1[1] > 0 )
			yaw = 90;
		else
			yaw = 270;

		if ( yaw < 0 )
			yaw += 360;

		forward = sqrtf( value1[0] * value1[0] + value1[1] * value1[1] );
		pitch = RAD2DEG( atan2f( value1[2], forward ) );
		if ( pitch < 0 )
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

/*
===============================================================================

	Dynamic light extensions

===============================================================================
*/

/*
===================
CL_Flashlight
===================
*/
void CL_Flashlight( int ent, vec3_t pos )
{
	cdlight_t *dl;

	dl = CL_AllocDlight( ent );
	VectorCopy( pos, dl->origin );
	dl->radius = 400;
	dl->minlight = 250;
	dl->die = (float)( cgi.time() + 100 );
	dl->color[0] = 1;
	dl->color[1] = 1;
	dl->color[2] = 1;
}

/*
===================
CL_ColorFlash

Flash of light
===================
*/
void CL_ColorFlash( vec3_t pos, int ent, int intensity, float r, float g, float b )
{
	cdlight_t *dl;

	dl = CL_AllocDlight( ent );
	VectorCopy( pos, dl->origin );
	dl->radius = (float)intensity;
	dl->minlight = 250;
	dl->die = (float)( cgi.time() + 100 );
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
}

/*
===============================================================================

	Particle extensions

===============================================================================
*/

/*
===================
CL_DebugTrail
===================
*/
void CL_DebugTrail( vec3_t start, vec3_t end )
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
//	int			j;
	cparticle_t *p;
	float		dec;
	vec3_t		right, up;
//	int			i;
//	float		d, c, s;
//	vec3_t		dir;

	VectorCopy( start, move );
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	MakeNormalVectors( vec, right, up );

//	VectorScale(vec, RT2_SKIP, vec);

//	dec = 1.0;
//	dec = 0.75;
	dec = 3;
	VectorScale( vec, dec, vec );
	VectorCopy( start, move );

	while ( len > 0 )
	{
		len -= dec;

		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		VectorClear( p->accel );
		VectorClear( p->vel );
		p->alpha = 1.0f;
		p->alphavel = -0.1f;
//		p->alphavel = 0;
		p->color = (float)( 0x74 + ( rand() & 7 ) );
		VectorCopy( move, p->org );
/*
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand()*2;
			p->vel[j] = crand()*3;
			p->accel[j] = 0;
		}
*/
		VectorAdd( move, vec, move );
	}

}

/*
===================
CL_SmokeTrail
===================
*/
void CL_SmokeTrail( vec3_t start, vec3_t end, int colorStart, int colorRun, int spacing )
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t *p;

	VectorCopy( start, move );
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	VectorScale( vec, (float)spacing, vec );

	// FIXME: this is a really silly way to have a loop
	while ( len > 0 )
	{
		len -= spacing;

		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear( p->accel );

		p->time = (float)cgi.time();

		p->alpha = 1.0f;
		p->alphavel = -1.0f / ( 1 + frand() * 0.5f );
		p->color = colorStart + ( rand() % colorRun );
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = move[j] + crand() * 3;
			p->accel[j] = 0;
		}
		p->vel[2] = 20 + crand() * 5;

		VectorAdd( move, vec, move );
	}
}

/*
===================
CL_ForceWall
===================
*/
void CL_ForceWall( vec3_t start, vec3_t end, int color )
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t *p;

	VectorCopy( start, move );
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	VectorScale( vec, 4, vec );

	// FIXME: this is a really silly way to have a loop
	while ( len > 0 )
	{
		len -= 4;

		if ( !free_particles )
			return;

		if ( frand() > 0.3f )
		{
			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;
			VectorClear( p->accel );

			p->time = (float)cgi.time();

			p->alpha = 1.0f;
			p->alphavel = -1.0f / ( 3.0f + frand() * 0.5f );
			p->color = (float)color;
			for ( j = 0; j < 3; j++ )
			{
				p->org[j] = move[j] + crand() * 3;
				p->accel[j] = 0;
			}
			p->vel[0] = 0;
			p->vel[1] = 0;
			p->vel[2] = -40 - ( crand() * 10 );
		}

		VectorAdd( move, vec, move );
	}
}

/*
===================
CL_FlameEffects
===================
*/
void CL_FlameEffects( centity_t *ent, vec3_t origin )
{
	int			n, count;
	int			j;
	cparticle_t *p;

	count = rand() & 0xF;

	for ( n = 0; n < count; n++ )
	{
		if ( !free_particles )
			return;

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		VectorClear( p->accel );
		p->time = (float)cgi.time();

		p->alpha = 1.0f;
		p->alphavel = -1.0f / ( 1 + frand() * 0.2f );
		p->color = (float)( 226 + ( rand() % 4 ) );
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = origin[j] + crand() * 5;
			p->vel[j] = crand() * 5;
		}
		p->vel[2] = crand() * -10;
		p->accel[2] = -PARTICLE_GRAVITY;
	}

	count = rand() & 0x7;

	for ( n = 0; n < count; n++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear( p->accel );

		p->time = (float)cgi.time();

		p->alpha = 1.0f;
		p->alphavel = -1.0f / ( 1 + frand() * 0.5f );
		p->color = 0 + ( rand() % 4 );
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = origin[j] + crand() * 3;
		}
		p->vel[2] = 20 + crand() * 5;
	}

}

/*
===================
CL_GenericParticleEffect
===================
*/
void CL_GenericParticleEffect( vec3_t org, vec3_t dir, int color, int count, int numcolors, int dirspread, float alphavel )
{
	int			i, j;
	cparticle_t *p;
	float		d;

	for ( i = 0; i < count; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		if ( numcolors > 1 )
			p->color = (float)( color + ( rand() & numcolors ) );
		else
			p->color = (float)color;

		d = (float)( rand() & dirspread );
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + ( ( rand() & 7 ) - 4 ) + d * dir[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
//		VectorCopy (accel, p->accel);
		p->alpha = 1.0f;

		p->alphavel = -1.0f / ( 0.5f + frand() * alphavel );
//		p->alphavel = alphavel;
	}
}

/*
===================
CL_BubbleTrail2

Lets you control the # of bubbles by setting the distance between the spawns
===================
*/
void CL_BubbleTrail2( vec3_t start, vec3_t end, int dist )
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			i, j;
	cparticle_t *p;
	int			dec;

	VectorCopy( start, move );
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	dec = dist;
	VectorScale( vec, (float)dec, vec );

	for ( i = 0; i < len; i += dec )
	{
		if ( !free_particles )
			return;

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		VectorClear( p->accel );
		p->time = (float)cgi.time();

		p->alpha = 1.0f;
		p->alphavel = -1.0f / ( 1 + frand() * 0.1f );
		p->color = 4 + ( rand() & 7 );
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = move[j] + crand() * 2;
			p->vel[j] = crand() * 10;
		}
		p->org[2] -= 4;
//		p->vel[2] += 6;
		p->vel[2] += 20;

		VectorAdd( move, vec, move );
	}
}

/*
===================
CL_Heatbeam

Slart: There were a lot of variations of this effect
but I removed them for the sake of keeping things clean
just check an earlier commit for them

void CL_Heatbeam (vec3_t start, vec3_t end)
===================
*/
void CL_Heatbeam( vec3_t start, vec3_t forward )
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t *p;
	vec3_t		right, up;
	int			i;
	float		c, s;
	vec3_t		dir;
	float		ltime;
	float		step = 32.0f, rstep;
	float		start_pt;
	float		rot;
	float		variance;
	vec3_t		end;

	VectorMA( start, 4096, forward, end );

	VectorCopy( start, move );
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	// FIXME - pmm - these might end up using old values?
//	MakeNormalVectors (vec, right, up);
	VectorCopy( *cgi.v_right(), right );
	VectorCopy( *cgi.v_up(), up );
	// GL mode
	VectorMA( move, -0.5f, right, move );
	VectorMA( move, -0.5f, up, move );
	// otherwise assume SOFT

	ltime = (float)cgi.time() / 1000.0f;
	start_pt = fmodf( ltime * 96.0f, step );
	VectorMA( move, start_pt, vec, move );

	VectorScale( vec, step, vec );

//	Com_Printf ("%f\n", ltime);
	rstep = M_PI_F / 10.0f;
	for ( i = start_pt; i < len; i += step )
	{
		if ( i > step * 5 ) // don't bother after the 5th ring
			break;

		for ( rot = 0; rot < mathconst::TwoPi; rot += rstep )
		{

			if ( !free_particles )
				return;

			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;

			p->time = (float)cgi.time();
			VectorClear( p->accel );
//			rot+= fmod(ltime, 12.0)*M_PI;
//			c = cos(rot)/2.0;
//			s = sin(rot)/2.0;
//			variance = 0.4 + ((float)rand()/(float)RAND_MAX) *0.2;
			variance = 0.5f;
			c = cosf( rot ) * variance;
			s = sinf( rot ) * variance;

			// trim it so it looks like it's starting at the origin
			if ( i < 10 )
			{
				VectorScale( right, c * ( i / 10.0f ), dir );
				VectorMA( dir, s * ( i / 10.0f ), up, dir );
			}
			else
			{
				VectorScale( right, c, dir );
				VectorMA( dir, s, up, dir );
			}

			p->alpha = 0.5f;
	//		p->alphavel = -1.0 / (1+frand()*0.2);
			p->alphavel = -1000.0f;
	//		p->color = 0x74 + (rand()&7);
			p->color = (float)( 223 - ( rand() & 7 ) );
			for ( j = 0; j < 3; j++ )
			{
				p->org[j] = move[j] + dir[j] * 3;
	//			p->vel[j] = dir[j]*6;
				p->vel[j] = 0;
			}
		}
		VectorAdd( move, vec, move );
	}
}

/*
===================
CL_ParticleSteamEffect

Puffs with velocity along direction, with some randomness thrown in
===================
*/
void CL_ParticleSteamEffect( vec3_t org, vec3_t dir, int color, int count, int magnitude )
{
	int			i, j;
	cparticle_t *p;
	float		d;
	vec3_t		r, u;

//	vectoangles2 (dir, angle_dir);
//	AngleVectors (angle_dir, f, r, u);

	MakeNormalVectors( dir, r, u );

	for ( i = 0; i < count; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		p->color = (float)( color + ( rand() & 7 ) );

		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + magnitude * 0.1f * crand();
//			p->vel[j] = dir[j]*magnitude;
		}
		VectorScale( dir, (float)magnitude, p->vel );
		d = crand() * magnitude / 3;
		VectorMA( p->vel, d, r, p->vel );
		d = crand() * magnitude / 3;
		VectorMA( p->vel, d, u, p->vel );

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY / 2;
		p->alpha = 1.0f;

		p->alphavel = -1.0f / ( 0.5f + frand() * 0.3f );
	}
}

/*
===================
CL_ParticleSteamEffect2
===================
*/
void CL_ParticleSteamEffect2( cl_sustain_t *self )
//vec3_t org, vec3_t dir, int color, int count, int magnitude)
{
	int			i, j;
	cparticle_t *p;
	float		d;
	vec3_t		r, u;
	vec3_t		dir;

//	vectoangles2 (dir, angle_dir);
//	AngleVectors (angle_dir, f, r, u);

	VectorCopy( self->dir, dir );
	MakeNormalVectors( dir, r, u );

	for ( i = 0; i < self->count; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		p->color = self->color + ( rand() & 7 );

		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = self->org[j] + self->magnitude * 0.1f * crand();
//			p->vel[j] = dir[j]*magnitude;
		}
		VectorScale( dir, self->magnitude, p->vel );
		d = crand() * self->magnitude / 3;
		VectorMA( p->vel, d, r, p->vel );
		d = crand() * self->magnitude / 3;
		VectorMA( p->vel, d, u, p->vel );

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY / 2;
		p->alpha = 1.0f;

		p->alphavel = -1.0f / ( 0.5f + frand() * 0.3f );
	}
	self->nextthink += self->thinkinterval;
}

/*
===================
CL_TrackerTrail
===================
*/
void CL_TrackerTrail( vec3_t start, vec3_t end, int particleColor )
{
	vec3_t		move;
	vec3_t		vec;
	vec3_t		forward, right, up, angle_dir;
	float		len;
	int			j;
	cparticle_t *p;
	int			dec;
	float		dist;

	VectorCopy( start, move );
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	VectorCopy( vec, forward );
	vectoangles2( forward, angle_dir );
	AngleVectors( angle_dir, forward, right, up );

	dec = 3;
	VectorScale( vec, 3, vec );

	// FIXME: this is a really silly way to have a loop
	while ( len > 0 )
	{
		len -= dec;

		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear( p->accel );

		p->time = (float)cgi.time();

		p->alpha = 1.0f;
		p->alphavel = -2.0f;
		p->color = (float)particleColor;
		dist = DotProduct( move, forward );
		VectorMA( move, 8 * cosf( dist ), up, p->org );
		for ( j = 0; j < 3; j++ )
		{
//			p->org[j] = move[j] + crand();
			p->vel[j] = 0;
			p->accel[j] = 0;
		}
		p->vel[2] = 5;

		VectorAdd( move, vec, move );
	}
}

/*
===================
CL_Tracker_Shell
===================
*/
void CL_Tracker_Shell( vec3_t origin )
{
	vec3_t			dir;
	int				i;
	cparticle_t		*p;

	for ( i = 0; i < 300; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear( p->accel );

		p->time = (float)cgi.time();

		p->alpha = 1.0f;
		p->alphavel = INSTANT_PARTICLE;
		p->color = 0;

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize( dir );

		VectorMA( origin, 40, dir, p->org );
	}
}

/*
===================
CL_MonsterPlasma_Shell
===================
*/
void CL_MonsterPlasma_Shell( vec3_t origin )
{
	vec3_t			dir;
	int				i;
	cparticle_t		*p;

	for ( i = 0; i < 40; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear( p->accel );

		p->time = (float)cgi.time();

		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = 0xe0;

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize( dir );

		VectorMA( origin, 10, dir, p->org );
//		VectorMA(origin, 10*(((rand () & 0x7fff) / ((float)0x7fff))), dir, p->org);
	}
}

/*
===================
CL_Widowbeamout
===================
*/
void CL_Widowbeamout( cl_sustain_t *self )
{
	vec3_t			dir;
	int				i;
	cparticle_t		*p;
	float			ratio;
	static float colortable[4] = {2*8,13*8,21*8,18*8};

	ratio = 1.0f - ( ( (float)self->endtime - (float)cgi.time() ) / 2100.0f );

	for ( i = 0; i < 300; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear( p->accel );

		p->time = (float)cgi.time();

		p->alpha = 1.0f;
		p->alphavel = INSTANT_PARTICLE;
		p->color = colortable[rand() & 3];

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize( dir );

		VectorMA( self->org, ( 45.0f * ratio ), dir, p->org );
//		VectorMA(origin, 10*(((rand () & 0x7fff) / ((float)0x7fff))), dir, p->org);
	}
}

/*
===================
CL_Nukeblast
===================
*/
void CL_Nukeblast( cl_sustain_t *self )
{
	vec3_t			dir;
	int				i;
	cparticle_t		*p;
	float			ratio;
	static float colortable[4] = { 110, 112, 114, 116 };

	ratio = 1.0f - ( ( (float)self->endtime - (float)cgi.time() ) / 1000.0f );

	for ( i = 0; i < 700; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear( p->accel );

		p->time = (float)cgi.time();

		p->alpha = 1.0f;
		p->alphavel = INSTANT_PARTICLE;
		p->color = colortable[rand() & 3];

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize( dir );

		VectorMA( self->org, ( 200.0f * ratio ), dir, p->org );
//		VectorMA(origin, 10*(((rand () & 0x7fff) / ((float)0x7fff))), dir, p->org);
	}
}

/*
===================
CL_WidowSplash
===================
*/
void CL_WidowSplash( vec3_t org )
{
	static float colortable[4] = {2*8,13*8,21*8,18*8};
	int			i;
	cparticle_t *p;
	vec3_t		dir;

	for ( i = 0; i < 256; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		p->color = colortable[rand() & 3];

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize( dir );
		VectorMA( org, 45.0f, dir, p->org );
		VectorMA( vec3_origin, 40.0f, dir, p->vel );

		p->accel[0] = p->accel[1] = 0;
		p->alpha = 1.0f;

		p->alphavel = -0.8f / ( 0.5f + frand() * 0.3f );
	}

}

/*
===================
CL_Tracker_Explode
===================
*/
void CL_Tracker_Explode( vec3_t	origin )
{
	vec3_t			dir, backdir;
	int				i;
	cparticle_t		*p;

	for ( i = 0; i < 300; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear( p->accel );

		p->time = (float)cgi.time();

		p->alpha = 1.0f;
		p->alphavel = -1.0f;
		p->color = 0;

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize( dir );
		VectorScale( dir, -1, backdir );

		VectorMA( origin, 64, dir, p->org );
		VectorScale( backdir, 64, p->vel );
	}

}

/*
===================
CL_TagTrail
===================
*/
void CL_TagTrail( vec3_t start, vec3_t end, float color )
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t *p;
	int			dec;

	VectorCopy( start, move );
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	dec = 5;
	VectorScale( vec, 5, vec );

	while ( len >= 0 )
	{
		len -= dec;

		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear( p->accel );

		p->time = (float)cgi.time();

		p->alpha = 1.0f;
		p->alphavel = -1.0f / ( 0.8f + frand() * 0.2f );
		p->color = color;
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = move[j] + crand() * 16;
			p->vel[j] = crand() * 5;
			p->accel[j] = 0;
		}

		VectorAdd( move, vec, move );
	}
}

/*
===================
CL_ColorExplosionParticles
===================
*/
void CL_ColorExplosionParticles( vec3_t org, int color, int run )
{
	int			i, j;
	cparticle_t *p;

	for ( i = 0; i < 128; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		p->color = (float)( color + ( rand() % run ) );

		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + ( ( rand() % 32 ) - 16 );
			p->vel[j] = (float)( ( rand() % 256 ) - 128 );
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0f;

		p->alphavel = -0.4f / ( 0.6f + frand() * 0.2f );
	}
}

/*
===================
CL_ParticleSmokeEffect

Like the steam effect, but unaffected by gravity
===================
*/
void CL_ParticleSmokeEffect( vec3_t org, vec3_t dir, int color, int count, int magnitude )
{
	int			i, j;
	cparticle_t *p;
	float		d;
	vec3_t		r, u;

	MakeNormalVectors( dir, r, u );

	for ( i = 0; i < count; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		p->color = (float)( color + ( rand() & 7 ) );

		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + magnitude * 0.1f * crand();
//			p->vel[j] = dir[j]*magnitude;
		}
		VectorScale( dir, magnitude, p->vel );
		d = crand() * magnitude / 3;
		VectorMA( p->vel, d, r, p->vel );
		d = crand() * magnitude / 3;
		VectorMA( p->vel, d, u, p->vel );

		p->accel[0] = p->accel[1] = p->accel[2] = 0;
		p->alpha = 1.0f;

		p->alphavel = -1.0f / ( 0.5f + frand() * 0.3f );
	}
}

/*
===================
CL_BlasterParticles2

Wall impact puffs (Green)
===================
*/
void CL_BlasterParticles2( vec3_t org, vec3_t dir, unsigned int color )
{
	int			i, j;
	cparticle_t *p;
	float		d;
	int			count;

	count = 40;
	for ( i = 0; i < count; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		p->color = (float)( color + ( rand() & 7 ) );

		d = (float)( rand() & 15 );
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + ( ( rand() & 7 ) - 4 ) + d * dir[j];
			p->vel[j] = dir[j] * 30 + crand() * 40;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0f;

		p->alphavel = -1.0f / ( 0.5f + frand() * 0.3f );
	}
}

/*
===============
CL_BlasterTrail2

Green!
===============
*/
void CL_BlasterTrail2( vec3_t start, vec3_t end )
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t *p;
	int			dec;

	VectorCopy( start, move );
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	dec = 5;
	VectorScale( vec, 5, vec );

	// FIXME: this is a really silly way to have a loop
	while ( len > 0 )
	{
		len -= dec;

		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear( p->accel );

		p->time = (float)cgi.time();

		p->alpha = 1.0f;
		p->alphavel = -1.0f / ( 0.3f + frand() * 0.2f );
		p->color = 0xd0;
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = move[j] + crand();
			p->vel[j] = crand() * 5;
			p->accel[j] = 0;
		}

		VectorAdd( move, vec, move );
	}
}
