/*
===============================================================================

	Client side temporary entities

	A surprising amount of this code blows chunks

===============================================================================
*/

#include "cg_local.h"

//
// Explosions
//

enum exptype_t
{
	ex_free, ex_explosion, ex_misc, ex_flash, ex_mflash, ex_poly, ex_poly2
};

struct explosion_t
{
	exptype_t	type;
	entity_t	ent;

	int			frames;
	float		light;
	vec3_t		lightcolor;
	float		start;
	int			baseframe;
};

#define	MAX_EXPLOSIONS	32
explosion_t cl_explosions[MAX_EXPLOSIONS];

//
// Beams
//

struct beam_t
{
	int		entity;
	int		dest_entity;
	model_t	*model;
	int		endtime;
	vec3_t	offset;
	vec3_t	start, end;
};

#define	MAX_BEAMS	32
beam_t cl_beams[MAX_BEAMS];
//PMM - added this for player-linked beams.  Currently only used by the plasma beam
beam_t cl_playerbeams[MAX_BEAMS];

//
// Lasers
//

struct laser_t
{
	entity_t	ent;
	int			endtime;
};

#define	MAX_LASERS	32
laser_t		cl_lasers[MAX_LASERS];

//
// Sustained particles
//

//ROGUE
cl_sustain_t	cl_sustains[MAX_SUSTAINS];
//ROGUE

//=============================================================================

//PGM
extern void CL_TeleportParticles( vec3_t org );
//PGM

void CL_BlasterParticles( vec3_t org, vec3_t dir );
void CL_ExplosionParticles( vec3_t org );
void CL_BFGExplosionParticles( vec3_t org );
// RAFAEL
void CL_BlueBlasterParticles( vec3_t org, vec3_t dir );

sfx_t	*cl_sfx_ric1;
sfx_t	*cl_sfx_ric2;
sfx_t	*cl_sfx_ric3;
sfx_t	*cl_sfx_lashit;
sfx_t	*cl_sfx_spark5;
sfx_t	*cl_sfx_spark6;
sfx_t	*cl_sfx_spark7;
sfx_t	*cl_sfx_railg;
sfx_t	*cl_sfx_rockexp;
sfx_t	*cl_sfx_grenexp;
sfx_t	*cl_sfx_watrexp;

model_t	*cl_mod_explode;
model_t	*cl_mod_smoke;
model_t	*cl_mod_flash;
model_t	*cl_mod_parasite_segment;
model_t	*cl_mod_grapple_cable;
model_t	*cl_mod_parasite_tip;
model_t	*cl_mod_explo4;
model_t	*cl_mod_bfg_explo;

//ROGUE
sfx_t	*cl_sfx_lightning;
sfx_t	*cl_sfx_disrexp;
model_t	*cl_mod_lightning;
model_t	*cl_mod_heatbeam;
model_t	*cl_mod_monster_heatbeam;
model_t	*cl_mod_explo4_big;

//RAFAEL
sfx_t	*cl_sfx_plasexp;
model_t	*cl_mod_plasmaexplo;

//=============================================================================

//ROGUE
/*
===================
CL_RegisterTEntSounds

Registers all sounds needed
===================
*/
void CL_RegisterTEntSounds()
{
	cl_sfx_ric1 = cgi.RegisterSound( "world/ric1.wav" );
	cl_sfx_ric2 = cgi.RegisterSound( "world/ric2.wav" );
	cl_sfx_ric3 = cgi.RegisterSound( "world/ric3.wav" );
	cl_sfx_lashit = cgi.RegisterSound( "weapons/lashit.wav" );
	cl_sfx_spark5 = cgi.RegisterSound( "world/spark5.wav" );
	cl_sfx_spark6 = cgi.RegisterSound( "world/spark6.wav" );
	cl_sfx_spark7 = cgi.RegisterSound( "world/spark7.wav" );
	cl_sfx_railg = cgi.RegisterSound( "weapons/railgf1a.wav" );
	cl_sfx_rockexp = cgi.RegisterSound( "weapons/rocklx1a.wav" );
	cl_sfx_grenexp = cgi.RegisterSound( "weapons/small_explosion1.ogg" );
	cl_sfx_watrexp = cgi.RegisterSound( "weapons/xpld_wat.wav" );
	// RAFAEL
	// cl_sfx_plasexp = cgi.RegisterSound ("weapons/plasexpl.wav");
	cgi.RegisterSound( "player/land1.wav" );

	cgi.RegisterSound( "player/fall2.wav" );
	cgi.RegisterSound( "player/fall1.wav" );

//PGM
	cl_sfx_lightning = cgi.RegisterSound( "weapons/tesla.wav" );
	cl_sfx_disrexp = cgi.RegisterSound( "weapons/disrupthit.wav" );
//PGM
}

/*
===================
CL_RegisterTEntModels

Registers all models needed
===================
*/
void CL_RegisterTEntModels()
{
	cl_mod_explode = cgi.RegisterModel( "models/objects/explode/tris.md2" );
	cl_mod_smoke = cgi.RegisterModel( "models/objects/smoke/tris.md2" );
	cl_mod_flash = cgi.RegisterModel( "models/objects/flash/tris.md2" );
	cl_mod_parasite_segment = cgi.RegisterModel( "models/monsters/parasite/segment/tris.md2" );
	cl_mod_grapple_cable = cgi.RegisterModel( "models/ctf/segment/tris.md2" );
	cl_mod_parasite_tip = cgi.RegisterModel( "models/monsters/parasite/tip/tris.md2" );
	cl_mod_explo4 = cgi.RegisterModel( "models/objects/r_explode/tris.md2" );
	cl_mod_bfg_explo = cgi.RegisterModel( "sprites/s_bfg2.sp2" );

	cgi.RegisterModel( "models/objects/laser/tris.md2" );
	cgi.RegisterModel( "models/objects/grenade2/tris.md2" );
	cgi.RegisterModel( "models/weapons/v_machn/tris.md2" );
	cgi.RegisterModel( "models/weapons/v_handgr/tris.md2" );
	cgi.RegisterModel( "models/weapons/v_shotg2/tris.md2" );
	cgi.RegisterModel( "models/objects/gibs/bone/tris.md2" );
	cgi.RegisterModel( "models/objects/gibs/sm_meat/tris.md2" );
	cgi.RegisterModel( "models/objects/gibs/bone2/tris.md2" );
// RAFAEL
// cgi.RegisterModel ("models/objects/blaser/tris.md2");

	cgi.RegisterPic( "w_machinegun" );
	cgi.RegisterPic( "a_bullets" );
	cgi.RegisterPic( "i_health" );
	cgi.RegisterPic( "a_grenades" );

//ROGUE
	cl_mod_explo4_big = cgi.RegisterModel( "models/objects/r_explode2/tris.md2" );
	cl_mod_lightning = cgi.RegisterModel( "models/proj/lightning/tris.md2" );
	cl_mod_heatbeam = cgi.RegisterModel( "models/proj/beam/tris.md2" );
	cl_mod_monster_heatbeam = cgi.RegisterModel( "models/proj/widowbeam/tris.md2" );
//ROGUE
}

/*
===================
CL_ClearTEnts
===================
*/
void CL_ClearTEnts()
{
	memset( cl_beams, 0, sizeof( cl_beams ) );
	memset( cl_explosions, 0, sizeof( cl_explosions ) );
	memset( cl_lasers, 0, sizeof( cl_lasers ) );

//ROGUE
	memset( cl_playerbeams, 0, sizeof( cl_playerbeams ) );
	memset( cl_sustains, 0, sizeof( cl_sustains ) );
//ROGUE
}

/*
===================
CL_AllocExplosion
===================
*/
explosion_t *CL_AllocExplosion()
{
	int		i;
	int		time;
	int		index;

	for ( i = 0; i < MAX_EXPLOSIONS; i++ )
	{
		if ( cl_explosions[i].type == ex_free )
		{
			memset( &cl_explosions[i], 0, sizeof( cl_explosions[i] ) );
			return &cl_explosions[i];
		}
	}
// find the oldest explosion
	time = cgi.time();
	index = 0;

	for ( i = 0; i < MAX_EXPLOSIONS; i++ )
	{
		if ( cl_explosions[i].start < time )
		{
			time = (int)cl_explosions[i].start;
			index = i;
		}
	}
	memset( &cl_explosions[index], 0, sizeof( cl_explosions[index] ) );
	return &cl_explosions[index];
}

/*
===================
CL_SmokeAndFlash
===================
*/
void CL_SmokeAndFlash( vec3_t origin )
{
	explosion_t *ex;

	ex = CL_AllocExplosion();
	VectorCopy( origin, ex->ent.origin );
	ex->type = ex_misc;
	ex->frames = 4;
	ex->ent.flags = RF_TRANSLUCENT;
	ex->start = (float)( cgi.servertime() - 100 );
	ex->ent.model = cl_mod_smoke;

	ex = CL_AllocExplosion();
	VectorCopy( origin, ex->ent.origin );
	ex->type = ex_flash;
	ex->ent.flags = RF_FULLBRIGHT;
	ex->frames = 2;
	ex->start = (float)( cgi.servertime() - 100 );
	ex->ent.model = cl_mod_flash;
}

/*
===================
CL_ParseParticles
===================
*/
void CL_ParseParticles()
{
	int		color, count;
	vec3_t	pos, dir;

	cgi.ReadPos( cgi.net_message, pos );
	cgi.ReadDir( cgi.net_message, dir );

	color = cgi.ReadByte( cgi.net_message );

	count = cgi.ReadByte( cgi.net_message );

	CL_ParticleEffect( pos, dir, color, count );
}

/*
===================
CL_ParseBeam
===================
*/
int CL_ParseBeam( model_t *model )
{
	int		ent;
	vec3_t	start, end;
	beam_t	*b;
	int		i;

	ent = cgi.ReadShort( cgi.net_message );

	cgi.ReadPos( cgi.net_message, start );
	cgi.ReadPos( cgi.net_message, end );

// override any beam with the same entity
	for ( i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++ )
		if ( b->entity == ent )
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cgi.time() + 200;
			VectorCopy( start, b->start );
			VectorCopy( end, b->end );
			VectorClear( b->offset );
			return ent;
		}

// find a free beam
	for ( i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++ )
	{
		if ( !b->model || b->endtime < cgi.time() )
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cgi.time() + 200;
			VectorCopy( start, b->start );
			VectorCopy( end, b->end );
			VectorClear( b->offset );
			return ent;
		}
	}
	Com_Printf( "beam list overflow!\n" );
	return ent;
}

/*
===================
CL_ParseBeam2
===================
*/
int CL_ParseBeam2( model_t *model )
{
	int		ent;
	vec3_t	start, end, offset;
	beam_t	*b;
	int		i;

	ent = cgi.ReadShort( cgi.net_message );

	cgi.ReadPos( cgi.net_message, start );
	cgi.ReadPos( cgi.net_message, end );
	cgi.ReadPos( cgi.net_message, offset );

//	Com_Printf ("end- %f %f %f\n", end[0], end[1], end[2]);

// override any beam with the same entity

	for ( i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++ )
		if ( b->entity == ent )
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cgi.time() + 200;
			VectorCopy( start, b->start );
			VectorCopy( end, b->end );
			VectorCopy( offset, b->offset );
			return ent;
		}

// find a free beam
	for ( i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++ )
	{
		if ( !b->model || b->endtime < cgi.time() )
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cgi.time() + 200;
			VectorCopy( start, b->start );
			VectorCopy( end, b->end );
			VectorCopy( offset, b->offset );
			return ent;
		}
	}
	Com_Printf( "beam list overflow!\n" );
	return ent;
}

// ROGUE
/*
===================
CL_ParsePlayerBeam

Adds to the cl_playerbeam array instead of the cl_beams array
===================
*/
int CL_ParsePlayerBeam( model_t *model )
{
	int		ent;
	vec3_t	start, end, offset;
	beam_t	*b;
	int		i;

	ent = cgi.ReadShort( cgi.net_message );

	cgi.ReadPos( cgi.net_message, start );
	cgi.ReadPos( cgi.net_message, end );
	// PMM - network optimization
	if ( model == cl_mod_heatbeam )
		VectorSet( offset, 2, 7, -3 );
	else if ( model == cl_mod_monster_heatbeam )
	{
		model = cl_mod_heatbeam;
		VectorSet( offset, 0, 0, 0 );
	}
	else
		cgi.ReadPos( cgi.net_message, offset );

//	Com_Printf ("end- %f %f %f\n", end[0], end[1], end[2]);

// override any beam with the same entity
// PMM - For player beams, we only want one per player (entity) so..
	for ( i = 0, b = cl_playerbeams; i < MAX_BEAMS; i++, b++ )
	{
		if ( b->entity == ent )
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cgi.time() + 200;
			VectorCopy( start, b->start );
			VectorCopy( end, b->end );
			VectorCopy( offset, b->offset );
			return ent;
		}
	}

// find a free beam
	for ( i = 0, b = cl_playerbeams; i < MAX_BEAMS; i++, b++ )
	{
		if ( !b->model || b->endtime < cgi.time() )
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cgi.time() + 100;		// PMM - this needs to be 100 to prevent multiple heatbeams
			VectorCopy( start, b->start );
			VectorCopy( end, b->end );
			VectorCopy( offset, b->offset );
			return ent;
		}
	}
	Com_Printf( "beam list overflow!\n" );
	return ent;
}
//ROGUE

/*
===================
CL_ParseLightning
===================
*/
int CL_ParseLightning( model_t *model )
{
	int		srcEnt, destEnt;
	vec3_t	start, end;
	beam_t	*b;
	int		i;

	srcEnt = cgi.ReadShort( cgi.net_message );
	destEnt = cgi.ReadShort( cgi.net_message );

	cgi.ReadPos( cgi.net_message, start );
	cgi.ReadPos( cgi.net_message, end );

// override any beam with the same source AND destination entities
	for ( i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++ )
	{
		if ( b->entity == srcEnt && b->dest_entity == destEnt )
		{
//			Com_Printf("%d: OVERRIDE  %d -> %d\n", cgi.time(), srcEnt, destEnt);
			b->entity = srcEnt;
			b->dest_entity = destEnt;
			b->model = model;
			b->endtime = cgi.time() + 200;
			VectorCopy( start, b->start );
			VectorCopy( end, b->end );
			VectorClear( b->offset );
			return srcEnt;
		}
	}

// find a free beam
	for ( i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++ )
	{
		if ( !b->model || b->endtime < cgi.time() )
		{
//			Com_Printf("%d: NORMAL  %d -> %d\n", cgi.time(), srcEnt, destEnt);
			b->entity = srcEnt;
			b->dest_entity = destEnt;
			b->model = model;
			b->endtime = cgi.time() + 200;
			VectorCopy( start, b->start );
			VectorCopy( end, b->end );
			VectorClear( b->offset );
			return srcEnt;
		}
	}
	Com_Printf( "beam list overflow!\n" );
	return srcEnt;
}

/*
===================
CL_ParseLaser
===================
*/
void CL_ParseLaser( int colors )
{
	vec3_t	start;
	vec3_t	end;
	laser_t	*l;
	int		i;

	cgi.ReadPos( cgi.net_message, start );
	cgi.ReadPos( cgi.net_message, end );

	for ( i = 0, l = cl_lasers; i < MAX_LASERS; i++, l++ )
	{
		if ( l->endtime < cgi.time() )
		{
			l->ent.flags = RF_TRANSLUCENT | RF_BEAM;
			VectorCopy( start, l->ent.origin );
			VectorCopy( end, l->ent.oldorigin );
			l->ent.alpha = 0.30;
			l->ent.skinnum = ( colors >> ( ( rand() % 4 ) * 8 ) ) & 0xff;
			l->ent.model = NULL;
			l->ent.frame = 4;
			l->endtime = cgi.time() + 100;
			return;
		}
	}
}

//=============
//ROGUE
void CL_ParseSteam()
{
	vec3_t	pos, dir;
	int		id, i;
	int		r;
	int		cnt;
	int		color;
	int		magnitude;
	cl_sustain_t	*s, *free_sustain;

	id = cgi.ReadShort( cgi.net_message );		// an id of -1 is an instant effect
	if ( id != -1 ) // sustains
	{
//			Com_Printf ("Sustain effect id %d\n", id);
		free_sustain = NULL;
		for ( i = 0, s = cl_sustains; i < MAX_SUSTAINS; i++, s++ )
		{
			if ( s->id == 0 )
			{
				free_sustain = s;
				break;
			}
		}
		if ( free_sustain )
		{
			s->id = id;
			s->count = cgi.ReadByte( cgi.net_message );
			cgi.ReadPos( cgi.net_message, s->org );
			cgi.ReadDir( cgi.net_message, s->dir );
			r = cgi.ReadByte( cgi.net_message );
			s->color = r & 0xff;
			s->magnitude = cgi.ReadShort( cgi.net_message );
			s->endtime = cgi.time() + cgi.ReadLong( cgi.net_message );
			s->think = CL_ParticleSteamEffect2;
			s->thinkinterval = 100;
			s->nextthink = cgi.time();
		}
		else
		{
//				Com_Printf ("No free sustains!\n");
			// FIXME - read the stuff anyway
			cnt = cgi.ReadByte( cgi.net_message );
			cgi.ReadPos( cgi.net_message, pos );
			cgi.ReadDir( cgi.net_message, dir );
			r = cgi.ReadByte( cgi.net_message );
			magnitude = cgi.ReadShort( cgi.net_message );
			magnitude = cgi.ReadLong( cgi.net_message ); // really interval
		}
	}
	else // instant
	{
		cnt = cgi.ReadByte( cgi.net_message );
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
		r = cgi.ReadByte( cgi.net_message );
		magnitude = cgi.ReadShort( cgi.net_message );
		color = r & 0xff;
		CL_ParticleSteamEffect( pos, dir, color, cnt, magnitude );
//		cgi.StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
	}
}

void CL_ParseWidow()
{
	vec3_t	pos;
	int		id, i;
	cl_sustain_t	*s, *free_sustain;

	id = cgi.ReadShort( cgi.net_message );

	free_sustain = NULL;
	for ( i = 0, s = cl_sustains; i < MAX_SUSTAINS; i++, s++ )
	{
		if ( s->id == 0 )
		{
			free_sustain = s;
			break;
		}
	}
	if ( free_sustain )
	{
		s->id = id;
		cgi.ReadPos( cgi.net_message, s->org );
		s->endtime = cgi.time() + 2100;
		s->think = CL_Widowbeamout;
		s->thinkinterval = 1;
		s->nextthink = cgi.time();
	}
	else // no free sustains
	{
		// FIXME - read the stuff anyway
		cgi.ReadPos( cgi.net_message, pos );
	}
}

void CL_ParseNuke()
{
	vec3_t	pos;
	int		i;
	cl_sustain_t	*s, *free_sustain;

	free_sustain = NULL;
	for ( i = 0, s = cl_sustains; i < MAX_SUSTAINS; i++, s++ )
	{
		if ( s->id == 0 )
		{
			free_sustain = s;
			break;
		}
	}
	if ( free_sustain )
	{
		s->id = 21000;
		cgi.ReadPos( cgi.net_message, s->org );
		s->endtime = cgi.time() + 1000;
		s->think = CL_Nukeblast;
		s->thinkinterval = 1;
		s->nextthink = cgi.time();
	}
	else // no free sustains
	{
		// FIXME - read the stuff anyway
		cgi.ReadPos( cgi.net_message, pos );
	}
}

//ROGUE
//=============


/*
===================
CL_ParseTEnt
===================
*/
void CL_ParseTEnt()
{
	static byte splash_color[] = { 0x00, 0xe0, 0xb0, 0x50, 0xd0, 0xe0, 0xe8 };
	int		type;
	vec3_t	pos, pos2, dir;
	explosion_t	*ex;
	int		cnt;
	int		color;
	int		r;
	int		ent;
	int		magnitude;

	type = cgi.ReadByte( cgi.net_message );

	switch ( type )
	{
	case TE_BLOOD:			// bullet hitting flesh
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
		CL_ParticleEffect( pos, dir, 0xe8, 60 );
		break;

	case TE_GUNSHOT:			// bullet hitting wall
	case TE_SPARKS:
	case TE_BULLET_SPARKS:
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
		if ( type == TE_GUNSHOT )
			CL_ParticleEffect( pos, dir, 0, 40 );
		else
			CL_ParticleEffect( pos, dir, 0xe0, 6 );

		if ( type != TE_SPARKS )
		{
			CL_SmokeAndFlash( pos );

			// impact sound
			cnt = rand() & 15;
			if ( cnt == 1 )
				cgi.StartSound( pos, 0, 0, cl_sfx_ric1, 1, ATTN_NORM, 0 );
			else if ( cnt == 2 )
				cgi.StartSound( pos, 0, 0, cl_sfx_ric2, 1, ATTN_NORM, 0 );
			else if ( cnt == 3 )
				cgi.StartSound( pos, 0, 0, cl_sfx_ric3, 1, ATTN_NORM, 0 );
		}

		break;

	case TE_SCREEN_SPARKS:
	case TE_SHIELD_SPARKS:
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
		if ( type == TE_SCREEN_SPARKS )
			CL_ParticleEffect( pos, dir, 0xd0, 40 );
		else
			CL_ParticleEffect( pos, dir, 0xb0, 40 );
		//FIXME : replace or remove this sound
		cgi.StartSound( pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0 );
		break;

	case TE_SHOTGUN:			// bullet hitting wall
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
		CL_ParticleEffect( pos, dir, 0, 20 );
		CL_SmokeAndFlash( pos );
		break;

	case TE_SPLASH:			// bullet hitting water
		cnt = cgi.ReadByte( cgi.net_message );
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
		r = cgi.ReadByte( cgi.net_message );
		if ( r > 6 )
			color = 0x00;
		else
			color = splash_color[r];
		CL_ParticleEffect( pos, dir, color, cnt );

		if ( r == SPLASH_SPARKS )
		{
			r = rand() & 3;
			if ( r == 0 )
				cgi.StartSound( pos, 0, 0, cl_sfx_spark5, 1, ATTN_STATIC, 0 );
			else if ( r == 1 )
				cgi.StartSound( pos, 0, 0, cl_sfx_spark6, 1, ATTN_STATIC, 0 );
			else
				cgi.StartSound( pos, 0, 0, cl_sfx_spark7, 1, ATTN_STATIC, 0 );
		}
		break;

	case TE_LASER_SPARKS:
		cnt = cgi.ReadByte( cgi.net_message );
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
		color = cgi.ReadByte( cgi.net_message );
		CL_ParticleEffect2( pos, dir, color, cnt );
		break;

	// RAFAEL
	case TE_BLUEHYPERBLASTER:
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadPos( cgi.net_message, dir );
		CL_BlasterParticles( pos, dir );
		break;

	case TE_BLASTER:			// blaster hitting wall
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
		CL_BlasterParticles( pos, dir );

		ex = CL_AllocExplosion();
		VectorCopy( pos, ex->ent.origin );
		ex->ent.angles[0] = RAD2DEG( acosf( dir[2] ) );
	// PMM - fixed to correct for pitch of 0
		if ( dir[0] )
			ex->ent.angles[1] = RAD2DEG( atan2f( dir[1], dir[0] ) );
		else if ( dir[1] > 0 )
			ex->ent.angles[1] = 90;
		else if ( dir[1] < 0 )
			ex->ent.angles[1] = 270;
		else
			ex->ent.angles[1] = 0;

		ex->type = ex_misc;
		ex->ent.flags = RF_FULLBRIGHT | RF_TRANSLUCENT;
		ex->start = (float)( cgi.servertime() - 100 );
		ex->light = 150;
		ex->lightcolor[0] = 1;
		ex->lightcolor[1] = 1;
		ex->ent.model = cl_mod_explode;
		ex->frames = 4;
		cgi.StartSound( pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0 );
		break;

	case TE_RAILTRAIL:			// railgun effect
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadPos( cgi.net_message, pos2 );
		CL_RailTrail( pos, pos2 );
		cgi.StartSound( pos2, 0, 0, cl_sfx_railg, 1, ATTN_NORM, 0 );
		break;

	case TE_EXPLOSION2:
	case TE_GRENADE_EXPLOSION:
	case TE_GRENADE_EXPLOSION_WATER:
		cgi.ReadPos( cgi.net_message, pos );

		ex = CL_AllocExplosion();
		VectorCopy( pos, ex->ent.origin );
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT;
		ex->start = (float)( cgi.servertime() - 100 );
		ex->light = 350;
		ex->lightcolor[0] = 1.0f;
		ex->lightcolor[1] = 0.5f;
		ex->lightcolor[2] = 0.5f;
		ex->ent.model = cl_mod_explo4;
		ex->frames = 19;
		ex->baseframe = 30;
		ex->ent.angles[1] = (float)( rand() % 360 );
		CL_ExplosionParticles( pos );
		if ( type == TE_GRENADE_EXPLOSION_WATER )
			cgi.StartSound( pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0 );
		else
			cgi.StartSound( pos, 0, 0, cl_sfx_grenexp, 1, ATTN_NORM, 0 );
		break;

	// RAFAEL
	case TE_PLASMA_EXPLOSION:
		cgi.ReadPos( cgi.net_message, pos );
		ex = CL_AllocExplosion();
		VectorCopy( pos, ex->ent.origin );
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT;
		ex->start = (float)( cgi.servertime() - 100 );
		ex->light = 350;
		ex->lightcolor[0] = 1.0f;
		ex->lightcolor[1] = 0.5f;
		ex->lightcolor[2] = 0.5f;
		ex->ent.angles[1] = (float)( rand() % 360 );
		ex->ent.model = cl_mod_explo4;
		if ( frand() < 0.5f )
			ex->baseframe = 15;
		ex->frames = 15;
		CL_ExplosionParticles( pos );
		cgi.StartSound( pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0 );
		break;

	case TE_EXPLOSION1:
	case TE_EXPLOSION1_BIG:						// PMM
	case TE_ROCKET_EXPLOSION:
	case TE_ROCKET_EXPLOSION_WATER:
	case TE_EXPLOSION1_NP:						// PMM
		cgi.ReadPos( cgi.net_message, pos );

		ex = CL_AllocExplosion();
		VectorCopy( pos, ex->ent.origin );
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT;
		ex->start = (float)( cgi.servertime() - 100 );
		ex->light = 350;
		ex->lightcolor[0] = 1.0f;
		ex->lightcolor[1] = 0.5f;
		ex->lightcolor[2] = 0.5f;
		ex->ent.angles[1] = (float)( rand() % 360 );
		if ( type != TE_EXPLOSION1_BIG )				// PMM
			ex->ent.model = cl_mod_explo4;			// PMM
		else
			ex->ent.model = cl_mod_explo4_big;
		if ( frand() < 0.5f )
			ex->baseframe = 15;
		ex->frames = 15;
		if ( ( type != TE_EXPLOSION1_BIG ) && ( type != TE_EXPLOSION1_NP ) )		// PMM
			CL_ExplosionParticles( pos );									// PMM
		if ( type == TE_ROCKET_EXPLOSION_WATER )
			cgi.StartSound( pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0 );
		else
			cgi.StartSound( pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0 );
		break;

	case TE_BFG_EXPLOSION:
		cgi.ReadPos( cgi.net_message, pos );
		ex = CL_AllocExplosion();
		VectorCopy( pos, ex->ent.origin );
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT;
		ex->start = (float)( cgi.servertime() - 100 );
		ex->light = 350;
		ex->lightcolor[0] = 0.0f;
		ex->lightcolor[1] = 1.0f;
		ex->lightcolor[2] = 0.0f;
		ex->ent.model = cl_mod_bfg_explo;
		ex->ent.flags |= RF_TRANSLUCENT;
		ex->ent.alpha = 0.30f;
		ex->frames = 4;
		break;

	case TE_BFG_BIGEXPLOSION:
		cgi.ReadPos( cgi.net_message, pos );
		CL_BFGExplosionParticles( pos );
		break;

	case TE_BFG_LASER:
		CL_ParseLaser( 0xd0d1d2d3 );
		break;

	case TE_BUBBLETRAIL:
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadPos( cgi.net_message, pos2 );
		CL_BubbleTrail( pos, pos2 );
		break;

	case TE_PARASITE_ATTACK:
	case TE_MEDIC_CABLE_ATTACK:
		ent = CL_ParseBeam( cl_mod_parasite_segment );
		break;

	case TE_BOSSTPORT:			// boss teleporting to station
		cgi.ReadPos( cgi.net_message, pos );
		CL_BigTeleportParticles( pos );
		cgi.StartSound( pos, 0, 0, cgi.RegisterSound( "misc/bigtele.wav" ), 1, ATTN_NONE, 0 );
		break;

	case TE_GRAPPLE_CABLE:
		ent = CL_ParseBeam2( cl_mod_grapple_cable );
		break;

	// RAFAEL
	case TE_WELDING_SPARKS:
		cnt = cgi.ReadByte( cgi.net_message );
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
		color = cgi.ReadByte( cgi.net_message );
		CL_ParticleEffect2( pos, dir, color, cnt );

		ex = CL_AllocExplosion();
		VectorCopy( pos, ex->ent.origin );
		ex->type = ex_flash;
		// note to self
		// we need a better no draw flag
		ex->ent.flags = RF_BEAM;
		ex->start = cgi.servertime() - 0.1f;
		ex->light = (float)( 100 + ( rand() % 75 ) );
		ex->lightcolor[0] = 1.0f;
		ex->lightcolor[1] = 1.0f;
		ex->lightcolor[2] = 0.3f;
		ex->ent.model = cl_mod_flash;
		ex->frames = 2;
		break;

	case TE_GREENBLOOD:
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
		CL_ParticleEffect2( pos, dir, 0xdf, 30 );
		break;

	// RAFAEL
	case TE_TUNNEL_SPARKS:
		cnt = cgi.ReadByte( cgi.net_message );
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
		color = cgi.ReadByte( cgi.net_message );
		CL_ParticleEffect3( pos, dir, color, cnt );
		break;

//=============
//PGM
		// PMM -following code integrated for flechette (different color)
	case TE_BLASTER2:			// green blaster hitting wall
	case TE_FLECHETTE:			// flechette
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );

		// PMM
		if ( type == TE_BLASTER2 )
			CL_BlasterParticles2( pos, dir, 0xd0 );
		else
			CL_BlasterParticles2( pos, dir, 0x6f ); // 75

		ex = CL_AllocExplosion();
		VectorCopy( pos, ex->ent.origin );
		ex->ent.angles[0] = RAD2DEG( acosf( dir[2] ) );
	// PMM - fixed to correct for pitch of 0
		if ( dir[0] )
			ex->ent.angles[1] = RAD2DEG( atan2f( dir[1], dir[0] ) );
		else if ( dir[1] > 0 )
			ex->ent.angles[1] = 90;
		else if ( dir[1] < 0 )
			ex->ent.angles[1] = 270;
		else
			ex->ent.angles[1] = 0;

		ex->type = ex_misc;
		ex->ent.flags = RF_FULLBRIGHT | RF_TRANSLUCENT;

		// PMM
		if ( type == TE_BLASTER2 )
			ex->ent.skinnum = 1;
		else // flechette
			ex->ent.skinnum = 2;

		ex->start = (float)( cgi.servertime() - 100 );
		ex->light = 150;
		// PMM
		if ( type == TE_BLASTER2 )
			ex->lightcolor[1] = 1;
		else // flechette
		{
			ex->lightcolor[0] = 0.19;
			ex->lightcolor[1] = 0.41;
			ex->lightcolor[2] = 0.75;
		}
		ex->ent.model = cl_mod_explode;
		ex->frames = 4;
		cgi.StartSound( pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0 );
		break;


	case TE_LIGHTNING:
		ent = CL_ParseLightning( cl_mod_lightning );
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cl_sfx_lightning, 1, ATTN_NORM, 0 );
		break;

	case TE_DEBUGTRAIL:
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadPos( cgi.net_message, pos2 );
		CL_DebugTrail( pos, pos2 );
		break;

	case TE_PLAIN_EXPLOSION:
		cgi.ReadPos( cgi.net_message, pos );

		ex = CL_AllocExplosion();
		VectorCopy( pos, ex->ent.origin );
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT;
		ex->start = (float)( cgi.servertime() - 100 );
		ex->light = 350;
		ex->lightcolor[0] = 1.0f;
		ex->lightcolor[1] = 0.5f;
		ex->lightcolor[2] = 0.5f;
		ex->ent.angles[1] = (float)( rand() % 360 );
		ex->ent.model = cl_mod_explo4;
		if ( frand() < 0.5f )
			ex->baseframe = 15;
		ex->frames = 15;
		if ( type == TE_ROCKET_EXPLOSION_WATER )
			cgi.StartSound( pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0 );
		else
			cgi.StartSound( pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0 );
		break;

	case TE_FLASHLIGHT:
		cgi.ReadPos( cgi.net_message, pos );
		ent = cgi.ReadShort( cgi.net_message );
		CL_Flashlight( ent, pos );
		break;

	case TE_FORCEWALL:
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadPos( cgi.net_message, pos2 );
		color = cgi.ReadByte( cgi.net_message );
		CL_ForceWall( pos, pos2, color );
		break;

	case TE_HEATBEAM:
		ent = CL_ParsePlayerBeam( cl_mod_heatbeam );
		break;

	case TE_MONSTER_HEATBEAM:
		ent = CL_ParsePlayerBeam( cl_mod_monster_heatbeam );
		break;

	case TE_HEATBEAM_SPARKS:
//		cnt = cgi.ReadByte (cgi.net_message);
		cnt = 50;
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
//		r = cgi.ReadByte (cgi.net_message);
//		magnitude = cgi.ReadShort (cgi.net_message);
		r = 8;
		magnitude = 60;
		color = r & 0xff;
		CL_ParticleSteamEffect( pos, dir, color, cnt, magnitude );
		cgi.StartSound( pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0 );
		break;

	case TE_HEATBEAM_STEAM:
//		cnt = cgi.ReadByte (cgi.net_message);
		cnt = 20;
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
//		r = cgi.ReadByte (cgi.net_message);
//		magnitude = cgi.ReadShort (cgi.net_message);
//		color = r & 0xff;
		color = 0xe0;
		magnitude = 60;
		CL_ParticleSteamEffect( pos, dir, color, cnt, magnitude );
		cgi.StartSound( pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0 );
		break;

	case TE_STEAM:
		CL_ParseSteam();
		break;

	case TE_BUBBLETRAIL2:
//		cnt = cgi.ReadByte (cgi.net_message);
		cnt = 8;
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadPos( cgi.net_message, pos2 );
		CL_BubbleTrail2( pos, pos2, cnt );
		cgi.StartSound( pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0 );
		break;

	case TE_MOREBLOOD:
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
		CL_ParticleEffect( pos, dir, 0xe8, 250 );
		break;

	case TE_CHAINFIST_SMOKE:
		dir[0] = 0; dir[1] = 0; dir[2] = 1;
		cgi.ReadPos( cgi.net_message, pos );
		CL_ParticleSmokeEffect( pos, dir, 0, 20, 20 );
		break;

	case TE_ELECTRIC_SPARKS:
		cgi.ReadPos( cgi.net_message, pos );
		cgi.ReadDir( cgi.net_message, dir );
//		CL_ParticleEffect (pos, dir, 109, 40);
		CL_ParticleEffect( pos, dir, 0x75, 40 );
		//FIXME : replace or remove this sound
		cgi.StartSound( pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0 );
		break;

	case TE_TRACKER_EXPLOSION:
		cgi.ReadPos( cgi.net_message, pos );
		CL_ColorFlash( pos, 0, 150, -1, -1, -1 );
		CL_ColorExplosionParticles( pos, 0, 1 );
//		CL_Tracker_Explode (pos);
		cgi.StartSound( pos, 0, 0, cl_sfx_disrexp, 1, ATTN_NORM, 0 );
		break;

	case TE_TELEPORT_EFFECT:
	case TE_DBALL_GOAL:
		cgi.ReadPos( cgi.net_message, pos );
		CL_TeleportParticles( pos );
		break;

	case TE_WIDOWBEAMOUT:
		CL_ParseWidow();
		break;

	case TE_NUKEBLAST:
		CL_ParseNuke();
		break;

	case TE_WIDOWSPLASH:
		cgi.ReadPos( cgi.net_message, pos );
		CL_WidowSplash( pos );
		break;
//PGM
//==============

	default:
		Com_Error( "CL_ParseTEnt: bad type" );
	}
}

/*
===================
CL_AddBeams
===================
*/
void CL_AddBeams()
{
	int			i, j;
	beam_t		*b;
	vec3_t		dist, org;
	float		d;
	entity_t	ent;
	float		yaw, pitch;
	float		forward;
	float		len, steps;
	float		model_length;

// update beams
	for ( i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++ )
	{
		if ( !b->model || b->endtime < cgi.time() )
			continue;

		// if coming from the player, update the start position
		if ( b->entity == cgi.playernum() + 1 )	// entity 0 is the world
		{
			VectorCopy( *cgi.vieworg(), b->start );
			b->start[2] -= 22;	// adjust for view height
		}
		VectorAdd( b->start, b->offset, org );

	// calculate pitch and yaw
		VectorSubtract( b->end, org, dist );

		if ( dist[1] == 0 && dist[0] == 0 )
		{
			yaw = 0;
			if ( dist[2] > 0 )
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
	// PMM - fixed to correct for pitch of 0
			if ( dist[0] )
				yaw = ( atan2f( dist[1], dist[0] ) * 180.0f / M_PI_F );
			else if ( dist[1] > 0 )
				yaw = 90;
			else
				yaw = 270;
			if ( yaw < 0 )
				yaw += 360;

			forward = sqrtf( dist[0] * dist[0] + dist[1] * dist[1] );
			pitch = ( atan2f( dist[2], forward ) * -180.0f / M_PI_F );
			if ( pitch < 0 )
				pitch += 360.0;
		}

	// add new entities for the beams
		d = VectorNormalize( dist );

		memset( &ent, 0, sizeof( ent ) );
		if ( b->model == cl_mod_lightning )
		{
			model_length = 35.0f;
			d -= 20.0f;  // correction so it doesn't end in middle of tesla
		}
		else
		{
			model_length = 30.0f;
		}
		steps = ceil( d / model_length );
		len = ( d - model_length ) / ( steps - 1 );

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ( ( b->model == cl_mod_lightning ) && ( d <= model_length ) )
		{
//			Com_Printf ("special case\n");
			VectorCopy( b->end, ent.origin );
			// offset to push beam outside of tesla model (negative because dist is from end to start
			// for this beam)
//			for (j=0 ; j<3 ; j++)
//				ent.origin[j] -= dist[j]*10.0;
			ent.model = b->model;
			ent.flags = RF_FULLBRIGHT;
			ent.angles[0] = pitch;
			ent.angles[1] = yaw;
			ent.angles[2] = (float)( rand() % 360 );
			cgi.AddEntity( &ent );
			return;
		}
		while ( d > 0 )
		{
			VectorCopy( org, ent.origin );
			ent.model = b->model;
			if ( b->model == cl_mod_lightning )
			{
				ent.flags = RF_FULLBRIGHT;
				ent.angles[0] = -pitch;
				ent.angles[1] = yaw + 180.0f;
				ent.angles[2] = (float)( rand() % 360 );
			}
			else
			{
				ent.angles[0] = pitch;
				ent.angles[1] = yaw;
				ent.angles[2] = (float)( rand() % 360 );
			}

//			Com_Printf("B: %d -> %d\n", b->entity, b->dest_entity);
			cgi.AddEntity( &ent );

			for ( j = 0; j < 3; j++ )
				org[j] += dist[j] * len;
			d -= model_length;
		}
	}
}


/*
//				Com_Printf ("Endpoint:  %f %f %f\n", b->end[0], b->end[1], b->end[2]);
//				Com_Printf ("Pred View Angles:  %f %f %f\n", cl.predicted_angles[0], cl.predicted_angles[1], cl.predicted_angles[2]);
//				Com_Printf ("Act View Angles: %f %f %f\n", cl.refdef.viewangles[0], cl.refdef.viewangles[1], cl.refdef.viewangles[2]);
//				VectorCopy (cl.predicted_origin, b->start);
//				b->start[2] += 22;	// adjust for view height
//				if (fabs(cgi.vieworg()[2] - b->start[2]) >= 10) {
//					b->start[2] = cgi.vieworg()[2];
//				}

//				Com_Printf ("Time:  %d %d %f\n", cgi.time(), cls.realtime, cls.frametime);
*/

/*
===================
ROGUE - draw player locked beams
CL_AddPlayerBeams
===================
*/
#if SLARTHACK
extern cvar_t *hand;
void CL_AddPlayerBeams( void )
{
	int			i, j;
	beam_t		*b;
	vec3_t		dist, org;
	float		d;
	entity_t	ent;
	float		yaw, pitch;
	float		forward;
	float		len, steps;
	int			framenum;
	float		model_length;

	float		hand_multiplier;
	frame_t		*oldframe;
	player_state_t	*ps, *ops;

//PMM
	if ( hand )
	{
		if ( hand->value == 2 )
			hand_multiplier = 0;
		else if ( hand->value == 1 )
			hand_multiplier = -1;
		else
			hand_multiplier = 1;
	}
	else
	{
		hand_multiplier = 1;
	}
//PMM

// update beams
	for ( i = 0, b = cl_playerbeams; i < MAX_BEAMS; i++, b++ )
	{
		vec3_t		f, r, u;
		if ( !b->model || b->endtime < cgi.time() )
			continue;

		if ( cl_mod_heatbeam && ( b->model == cl_mod_heatbeam ) )
		{

			// if coming from the player, update the start position
			if ( b->entity == cgi.playernum() + 1 )	// entity 0 is the world
			{
				// set up gun position
				// code straight out of CL_AddViewWeapon
				ps = cgi.playerstate();
				j = ( cgi.serverframe() - 1 ) & UPDATE_MASK;
				oldframe = &cl.frames[j];
				if ( oldframe->serverframe != cgi.serverframe() - 1 || !oldframe->valid )
					oldframe = &cl.frame;		// previous frame was dropped or involid
				ops = &oldframe->playerstate;
				for ( j = 0; j < 3; j++ )
				{
					b->start[j] = cgi.vieworg()[j] + ops->gunoffset[j]
						+ cl.lerpfrac * ( ps->gunoffset[j] - ops->gunoffset[j] );
				}
				VectorMA( b->start, ( hand_multiplier * b->offset[0] ), cl.v_right, org );
				VectorMA( org, b->offset[1], cl.v_forward, org );
				VectorMA( org, b->offset[2], cl.v_up, org );
				if ( ( hand ) && ( hand->value == 2 ) ) {
					VectorMA( org, -1, cl.v_up, org );
				}
				// FIXME - take these out when final
				VectorCopy( cl.v_right, r );
				VectorCopy( cl.v_forward, f );
				VectorCopy( cl.v_up, u );

			}
			else
				VectorCopy( b->start, org );
		}
		else
		{
			// if coming from the player, update the start position
			if ( b->entity == cgi.playernum() + 1 )	// entity 0 is the world
			{
				VectorCopy( cgi.vieworg(), b->start );
				b->start[2] -= 22;	// adjust for view height
			}
			VectorAdd( b->start, b->offset, org );
		}

	// calculate pitch and yaw
		VectorSubtract( b->end, org, dist );

//PMM
		if ( cl_mod_heatbeam && ( b->model == cl_mod_heatbeam ) && ( b->entity == cgi.playernum() + 1 ) )
		{
			vec_t len;

			len = VectorLength( dist );
			VectorScale( f, len, dist );
			VectorMA( dist, ( hand_multiplier * b->offset[0] ), r, dist );
			VectorMA( dist, b->offset[1], f, dist );
			VectorMA( dist, b->offset[2], u, dist );
			if ( ( hand ) && ( hand->value == 2 ) ) {
				VectorMA( org, -1, cl.v_up, org );
			}
		}
//PMM

		if ( dist[1] == 0 && dist[0] == 0 )
		{
			yaw = 0;
			if ( dist[2] > 0 )
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
	// PMM - fixed to correct for pitch of 0
			if ( dist[0] )
				yaw = ( atan2f( dist[1], dist[0] ) * 180.0f / M_PI_F );
			else if ( dist[1] > 0 )
				yaw = 90;
			else
				yaw = 270;
			if ( yaw < 0 )
				yaw += 360;

			forward = sqrtf( dist[0] * dist[0] + dist[1] * dist[1] );
			pitch = ( atan2f( dist[2], forward ) * -180.0f / M_PI_F );
			if ( pitch < 0 )
				pitch += 360.0f;
		}

		if ( cl_mod_heatbeam && ( b->model == cl_mod_heatbeam ) )
		{
			if ( b->entity != cgi.playernum() + 1 )
			{
				framenum = 2;
//				Com_Printf ("Third person\n");
				ent.angles[0] = -pitch;
				ent.angles[1] = yaw + 180.0f;
				ent.angles[2] = 0;
//				Com_Printf ("%f %f - %f %f %f\n", -pitch, yaw+180.0, b->offset[0], b->offset[1], b->offset[2]);
				AngleVectors( ent.angles, f, r, u );

				// if it's a non-origin offset, it's a player, so use the hardcoded player offset
				if ( !VectorCompare( b->offset, vec3_origin ) )
				{
					VectorMA( org, -( b->offset[0] ) + 1, r, org );
					VectorMA( org, -( b->offset[1] ), f, org );
					VectorMA( org, -( b->offset[2] ) - 10, u, org );
				}
				else
				{
					// if it's a monster, do the particle effect
					CL_MonsterPlasma_Shell( b->start );
				}
			}
			else
			{
				framenum = 1;
			}
		}

		// if it's the heatbeam, draw the particle effect
		if ( ( cl_mod_heatbeam && ( b->model == cl_mod_heatbeam ) && ( b->entity == cgi.playernum() + 1 ) ) )
		{
			CL_Heatbeam( org, dist );
		}

	// add new entities for the beams
		d = VectorNormalize( dist );

		memset( &ent, 0, sizeof( ent ) );
		if ( b->model == cl_mod_heatbeam )
		{
			model_length = 32.0;
		}
		else if ( b->model == cl_mod_lightning )
		{
			model_length = 35.0;
			d -= 20.0;  // correction so it doesn't end in middle of tesla
		}
		else
		{
			model_length = 30.0;
		}
		steps = ceil( d / model_length );
		len = ( d - model_length ) / ( steps - 1 );

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ( ( b->model == cl_mod_lightning ) && ( d <= model_length ) )
		{
//			Com_Printf ("special case\n");
			VectorCopy( b->end, ent.origin );
			// offset to push beam outside of tesla model (negative because dist is from end to start
			// for this beam)
//			for (j=0 ; j<3 ; j++)
//				ent.origin[j] -= dist[j]*10.0;
			ent.model = b->model;
			ent.flags = RF_FULLBRIGHT;
			ent.angles[0] = pitch;
			ent.angles[1] = yaw;
			ent.angles[2] = (float)( rand() % 360 );
			cgi.AddEntity( &ent );
			return;
		}
		while ( d > 0 )
		{
			VectorCopy( org, ent.origin );
			ent.model = b->model;
			if ( cl_mod_heatbeam && ( b->model == cl_mod_heatbeam ) )
			{
//				ent.flags = RF_FULLBRIGHT|RF_TRANSLUCENT;
//				ent.alpha = 0.3;
				ent.flags = RF_FULLBRIGHT;
				ent.angles[0] = -pitch;
				ent.angles[1] = yaw + 180.0f;
				ent.angles[2] = ( cgi.time() ) % 360;
//				ent.angles[2] = rand()%360;
				ent.frame = framenum;
			}
			else if ( b->model == cl_mod_lightning )
			{
				ent.flags = RF_FULLBRIGHT;
				ent.angles[0] = -pitch;
				ent.angles[1] = yaw + 180.0f;
				ent.angles[2] = (float)( rand() % 360 );
			}
			else
			{
				ent.angles[0] = pitch;
				ent.angles[1] = yaw;
				ent.angles[2] = (float)( rand() % 360 );
			}

//			Com_Printf("B: %d -> %d\n", b->entity, b->dest_entity);
			cgi.AddEntity( &ent );

			for ( j = 0; j < 3; j++ )
				org[j] += dist[j] * len;
			d -= model_length;
		}
	}
}
#endif

/*
===================
CL_AddExplosions
===================
*/
void CL_AddExplosions()
{
	entity_t	*ent;
	int			i;
	explosion_t	*ex;
	float		frac;
	int			f;

	memset( &ent, 0, sizeof( ent ) );

	for ( i = 0, ex = cl_explosions; i < MAX_EXPLOSIONS; i++, ex++ )
	{
		if ( ex->type == ex_free )
			continue;
		frac = ( cgi.time() - ex->start ) / 100.0f;
		f = floor( frac );

		ent = &ex->ent;

		switch ( ex->type )
		{
		case ex_mflash:
			if ( f >= ex->frames - 1 )
				ex->type = ex_free;
			break;
		case ex_misc:
			if ( f >= ex->frames - 1 )
			{
				ex->type = ex_free;
				break;
			}
			ent->alpha = 1.0f - frac / ( ex->frames - 1 );
			break;
		case ex_flash:
			if ( f >= 1 )
			{
				ex->type = ex_free;
				break;
			}
			ent->alpha = 1.0f;
			break;
		case ex_poly:
			if ( f >= ex->frames - 1 )
			{
				ex->type = ex_free;
				break;
			}

			ent->alpha = ( 16.0f - (float)f ) / 16.0f;

			if ( f < 10 )
			{
				ent->skinnum = ( f >> 1 );
				if ( ent->skinnum < 0 )
					ent->skinnum = 0;
			}
			else
			{
				ent->flags |= RF_TRANSLUCENT;
				if ( f < 13 )
					ent->skinnum = 5;
				else
					ent->skinnum = 6;
			}
			break;
		case ex_poly2:
			if ( f >= ex->frames - 1 )
			{
				ex->type = ex_free;
				break;
			}

			ent->alpha = ( 5.0f - (float)f ) / 5.0f;
			ent->skinnum = 0;
			ent->flags |= RF_TRANSLUCENT;
			break;
		}

		if ( ex->type == ex_free )
			continue;
		if ( ex->light )
		{
			cgi.AddLight( ent->origin, ex->light * ent->alpha,
				ex->lightcolor[0], ex->lightcolor[1], ex->lightcolor[2] );
		}

		VectorCopy( ent->origin, ent->oldorigin );

		if ( f < 0 )
			f = 0;
		ent->frame = ex->baseframe + f + 1;
		ent->oldframe = ex->baseframe + f;
		ent->backlerp = 1.0f - cgi.lerpfrac();

		cgi.AddEntity( ent );
	}
}


/*
===================
CL_AddLasers
===================
*/
void CL_AddLasers()
{
	laser_t		*l;
	int			i;

	for ( i = 0, l = cl_lasers; i < MAX_LASERS; i++, l++ )
	{
		if ( l->endtime >= cgi.time() )
			cgi.AddEntity( &l->ent );
	}
}

/* PMM - CL_Sustains */
/*
===================
CL_ProcessSustain
===================
*/
static void CL_ProcessSustain()
{
	cl_sustain_t	*s;
	int				i;

	for ( i = 0, s = cl_sustains; i < MAX_SUSTAINS; i++, s++ )
	{
		if ( s->id )
		{
			if ( ( s->endtime >= cgi.time() ) && ( cgi.time() >= s->nextthink ) )
			{
//				Com_Printf ("think %d %d %d\n", cgi.time(), s->nextthink, s->thinkinterval);
				s->think( s );
			}
			else if ( s->endtime < cgi.time() )
			{
				s->id = 0;
			}
		}
	}
}

/*
===================
CL_AddTEnts
===================
*/
void CL_AddTEnts()
{
	CL_AddBeams();
	// PMM - draw plasma beams
#if SLARTHACK
	CL_AddPlayerBeams();
#endif
	CL_AddExplosions();
	CL_AddLasers();
	// PMM - set up sustain
	CL_ProcessSustain();
}
