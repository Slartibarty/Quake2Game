/*
===============================================================================

	Entity effects parsing and management

===============================================================================
*/

#include "cg_local.h"

#define	BEAMLENGTH	16

void CL_LogoutEffect (vec3_t org, int type);
void CL_ItemRespawnParticles( vec3_t org );

static vec3_t avelocities[NUMVERTEXNORMALS];

/*
===============================================================================

	Light style management

===============================================================================
*/

// Slart: WTF is this?
struct clightstyle_t
{
	int		length;
	vec3_t	value;
	float	map[MAX_QPATH];
};

static clightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
static int				s_lastofs;

/*
===================
CL_ClearLightStyles
===================
*/
static void CL_ClearLightStyles()
{
	memset( cl_lightstyle, 0, sizeof( cl_lightstyle ) );
	s_lastofs = -1;
}

/*
===================
CL_RunLightStyles
===================
*/
void CL_RunLightStyles()
{
	int		ofs;
	int		i;
	clightstyle_t *ls;

	ofs = cgi.time() / 100;
	if ( ofs == s_lastofs )
		return;
	s_lastofs = ofs;

	for ( i = 0, ls = cl_lightstyle; i < MAX_LIGHTSTYLES; i++, ls++ )
	{
		if ( !ls->length )
		{
			ls->value[0] = ls->value[1] = ls->value[2] = 1.0;
			continue;
		}
		if ( ls->length == 1 )
		{
			ls->value[0] = ls->value[1] = ls->value[2] = ls->map[0];
		}
		else
		{
			ls->value[0] = ls->value[1] = ls->value[2] = ls->map[ofs % ls->length];
		}
	}
}

/*
===================
CL_SetLightstyle
===================
*/
void CL_SetLightstyle( int index, const char *s )
{
	strlen_t j, k;

	j = Q_strlen( s );
	if ( j >= MAX_QPATH )
	{
		Com_Errorf( "svc_lightstyle length=%i", j );
	}

	cl_lightstyle[index].length = j;

	for ( k = 0; k < j; ++k )
	{
		cl_lightstyle[index].map[k] = (float)( s[k] - 'a' ) / (float)( 'm' - 'a' );
	}
}

/*
===================
CL_AddLightStyles
===================
*/
void CL_AddLightStyles()
{
	int i;
	clightstyle_t *ls;

	for ( i = 0, ls = cl_lightstyle; i < MAX_LIGHTSTYLES; i++, ls++ )
	{
		cgi.AddLightStyle( i, ls->value[0], ls->value[1], ls->value[2] );
	}
}

/*
===============================================================================

	Dynamic light management

===============================================================================
*/

cdlight_t		cl_dlights[MAX_DLIGHTS];

/*
===================
CL_ClearDlights
===================
*/
static void CL_ClearDlights()
{
	memset( cl_dlights, 0, sizeof( cl_dlights ) );
}

/*
===================
CL_AllocDlight
===================
*/
cdlight_t *CL_AllocDlight( int key )
{
	int i;
	cdlight_t *dl;

	// first look for an exact key match
	if ( key )
	{
		dl = cl_dlights;
		for ( i = 0; i < MAX_DLIGHTS; i++, dl++ )
		{
			if ( dl->key == key )
			{
				memset( dl, 0, sizeof( *dl ) );
				dl->key = key;
				return dl;
			}
		}
	}

	// then look for anything else
	dl = cl_dlights;
	for ( i = 0; i < MAX_DLIGHTS; i++, dl++ )
	{
		if ( dl->die < cgi.time() )
		{
			memset( dl, 0, sizeof( *dl ) );
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	memset( dl, 0, sizeof( *dl ) );
	dl->key = key;
	return dl;
}

/*
===================
CL_NewDlight
===================
*/
static void CL_NewDlight( int key, float x, float y, float z, float radius, float time )
{
	cdlight_t *dl;

	dl = CL_AllocDlight( key );
	dl->origin[0] = x;
	dl->origin[1] = y;
	dl->origin[2] = z;
	dl->radius = radius;
	dl->die = cgi.time() + time;
}

/*
===================
CL_RunDLights
===================
*/
void CL_RunDLights()
{
	int i;
	cdlight_t *dl;

	dl = cl_dlights;
	for ( i = 0; i < MAX_DLIGHTS; i++, dl++ )
	{
		if ( !dl->radius )
		{
			continue;
		}

		if ( dl->die < cgi.time() )
		{
			dl->radius = 0;
			return;
		}
		dl->radius -= cgi.frametime() * dl->decay;
		if ( dl->radius < 0 )
		{
			dl->radius = 0;
		}
	}
}

/*
===================
CL_ParseMuzzleFlash
===================
*/
void CL_ParseMuzzleFlash()
{
	vec3_t		fv, rv;
	cdlight_t	*dl;
	int			i, weapon;
	centity_t	*pl;
	bool		silenced;
	float		volume;
	char		soundname[MAX_QPATH];

	i = cgi.ReadShort( cgi.net_message );
	if ( i < 1 || i >= MAX_EDICTS )
	{
		Com_Error( "CL_ParseMuzzleFlash: bad entity" );
	}

	weapon = cgi.ReadByte( cgi.net_message );
	silenced = weapon & MZ_SILENCED;
	weapon &= ~MZ_SILENCED;

	pl = cgi.GetEntityAtIndex( i );

	dl = CL_AllocDlight( i );
	VectorCopy( pl->current.origin, dl->origin );
	AngleVectors( pl->current.angles, fv, rv, NULL );
	VectorMA( dl->origin, 18, fv, dl->origin );
	VectorMA( dl->origin, 16, rv, dl->origin );
	if ( silenced )
	{
		dl->radius = (float)( 100 + ( rand() & 31 ) );
	}
	else
	{
		dl->radius = (float)( 200 + ( rand() & 31 ) );
	}
	dl->minlight = 32;
	dl->die = (float)cgi.time(); // + 0.1;

	if ( silenced )
	{
		volume = 0.2f;
	}
	else
	{
		volume = 1.0f;
	}

	switch ( weapon )
	{
	case MZ_BLASTER:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/blastf1a.wav" ), volume, ATTN_NORM, 0 );
		break;
	case MZ_BLUEHYPERBLASTER:
		dl->color[0] = 0; dl->color[1] = 0; dl->color[2] = 1;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/hyprbf1a.wav" ), volume, ATTN_NORM, 0 );
		break;
	case MZ_HYPERBLASTER:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/hyprbf1a.wav" ), volume, ATTN_NORM, 0 );
		break;
	case MZ_MACHINEGUN:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		Q_sprintf_s( soundname, "weapons/machgf%ib.wav", ( rand() % 5 ) + 1 );
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( soundname ), volume, ATTN_NORM, 0 );
		break;
	case MZ_SHOTGUN:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/shotgf1b.wav" ), volume, ATTN_NORM, 0 );
		cgi.StartSound( NULL, i, CHAN_AUTO, cgi.RegisterSound( "weapons/shotgr1b.wav" ), volume, ATTN_NORM, 0.1f );
		break;
	case MZ_SSHOTGUN:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/sshotf1b.wav" ), volume, ATTN_NORM, 0 );
		break;
	case MZ_CHAINGUN1:
		dl->radius = (float)( 200 + ( rand() & 31 ) );
		dl->color[0] = 1; dl->color[1] = 0.25f; dl->color[2] = 0;
		Q_sprintf_s( soundname, "weapons/machgf%ib.wav", ( rand() % 5 ) + 1 );
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( soundname ), volume, ATTN_NORM, 0 );
		break;
	case MZ_CHAINGUN2:
		dl->radius = (float)( 225 + ( rand() & 31 ) );
		dl->color[0] = 1; dl->color[1] = 0.5f; dl->color[2] = 0;
		dl->die = cgi.time() + 0.1f;	// long delay
		Q_sprintf_s( soundname, "weapons/machgf%ib.wav", ( rand() % 5 ) + 1 );
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( soundname ), volume, ATTN_NORM, 0 );
		Q_sprintf_s( soundname, "weapons/machgf%ib.wav", ( rand() % 5 ) + 1 );
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( soundname ), volume, ATTN_NORM, 0.05f );
		break;
	case MZ_CHAINGUN3:
		dl->radius = (float)( 250 + ( rand() & 31 ) );
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		dl->die = cgi.time() + 0.1f;	// long delay
		Q_sprintf_s( soundname, "weapons/machgf%ib.wav", ( rand() % 5 ) + 1 );
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( soundname ), volume, ATTN_NORM, 0 );
		Q_sprintf_s( soundname, "weapons/machgf%ib.wav", ( rand() % 5 ) + 1 );
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( soundname ), volume, ATTN_NORM, 0.033f );
		Q_sprintf_s( soundname, "weapons/machgf%ib.wav", ( rand() % 5 ) + 1 );
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( soundname ), volume, ATTN_NORM, 0.066f );
		break;
	case MZ_RAILGUN:
		dl->color[0] = 0.5f; dl->color[1] = 0.5f; dl->color[2] = 1.0f;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/railgf1a.wav" ), volume, ATTN_NORM, 0 );
		break;
	case MZ_ROCKET:
		dl->color[0] = 1; dl->color[1] = 0.5f; dl->color[2] = 0.2f;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/rocklf1a.wav" ), volume, ATTN_NORM, 0 );
		cgi.StartSound( NULL, i, CHAN_AUTO, cgi.RegisterSound( "weapons/rocklr1b.wav" ), volume, ATTN_NORM, 0.1f );
		break;
	case MZ_GRENADE:
		dl->color[0] = 1; dl->color[1] = 0.5f; dl->color[2] = 0;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/grenlf1a.wav" ), volume, ATTN_NORM, 0 );
		cgi.StartSound( NULL, i, CHAN_AUTO, cgi.RegisterSound( "weapons/grenlr1b.wav" ), volume, ATTN_NORM, 0.1f );
		break;
	case MZ_BFG:
		dl->color[0] = 0; dl->color[1] = 1; dl->color[2] = 0;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/bfg__f1y.wav" ), volume, ATTN_NORM, 0 );
		break;

	case MZ_LOGIN:
		dl->color[0] = 0; dl->color[1] = 1; dl->color[2] = 0;
		dl->die = cgi.time() + 1.0f;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/grenlf1a.wav" ), 1, ATTN_NORM, 0 );
		CL_LogoutEffect( pl->current.origin, weapon );
		break;
	case MZ_LOGOUT:
		dl->color[0] = 1; dl->color[1] = 0; dl->color[2] = 0;
		dl->die = cgi.time() + 1.0f;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/grenlf1a.wav" ), 1, ATTN_NORM, 0 );
		CL_LogoutEffect( pl->current.origin, weapon );
		break;
	case MZ_RESPAWN:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		dl->die = cgi.time() + 1.0f;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/grenlf1a.wav" ), 1, ATTN_NORM, 0 );
		CL_LogoutEffect( pl->current.origin, weapon );
		break;

	// RAFAEL
	case MZ_PHALANX:
		dl->color[0] = 1; dl->color[1] = 0.5f; dl->color[2] = 0.5f;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/plasshot.wav" ), volume, ATTN_NORM, 0 );
		break;
	// RAFAEL
	case MZ_IONRIPPER:
		dl->color[0] = 1; dl->color[1] = 0.5f; dl->color[2] = 0.5f;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/rippfire.wav" ), volume, ATTN_NORM, 0 );
		break;

//=================
// PGM
	case MZ_ETF_RIFLE:
		dl->color[0] = 0.9; dl->color[1] = 0.7f; dl->color[2] = 0;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/nail1.wav" ), volume, ATTN_NORM, 0 );
		break;
	case MZ_SHOTGUN2:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/shotg2.wav" ), volume, ATTN_NORM, 0 );
		break;
	case MZ_HEATBEAM:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		dl->die = (float)( cgi.time() + 100 );
		//cgi.StartSound (NULL, i, CHAN_WEAPON, cgi.RegisterSound("weapons/bfg__l1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_BLASTER2:
		dl->color[0] = 0; dl->color[1] = 1; dl->color[2] = 0;
		// FIXME - different sound for blaster2 ??
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/blastf1a.wav" ), volume, ATTN_NORM, 0 );
		break;
	case MZ_TRACKER:
		// negative flashes handled the same in gl/soft until CL_AddDLights
		dl->color[0] = -1; dl->color[1] = -1; dl->color[2] = -1;
		cgi.StartSound( NULL, i, CHAN_WEAPON, cgi.RegisterSound( "weapons/disint2.wav" ), volume, ATTN_NORM, 0 );
		break;
	case MZ_NUKE1:
		dl->color[0] = 1; dl->color[1] = 0; dl->color[2] = 0;
		dl->die = (float)( cgi.time() + 100 );
		break;
	case MZ_NUKE2:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		dl->die = (float)( cgi.time() + 100 );
		break;
	case MZ_NUKE4:
		dl->color[0] = 0; dl->color[1] = 0; dl->color[2] = 1;
		dl->die = (float)( cgi.time() + 100 );
		break;
	case MZ_NUKE8:
		dl->color[0] = 0; dl->color[1] = 1; dl->color[2] = 1;
		dl->die = (float)( cgi.time() + 100 );
		break;
// PGM
//=================
	}
}

/*
===================
CL_ParseMuzzleFlash2
===================
*/
void CL_ParseMuzzleFlash2()
{
	int			ent;
	vec3_t		origin;
	int			flash_number;
	cdlight_t	*dl;
	vec3_t		forward, right;
	char		soundname[64];

	ent = cgi.ReadShort( cgi.net_message );
	if ( ent < 1 || ent >= MAX_EDICTS )
	{
		Com_Error( "CL_ParseMuzzleFlash2: bad entity" );
	}

	flash_number = cgi.ReadByte( cgi.net_message );

	centity_t *self = cgi.GetEntityAtIndex( ent );

	// locate the origin
	AngleVectors( self->current.angles, forward, right, NULL );
	origin[0] = self->current.origin[0] + forward[0] * monster_flash_offset[flash_number][0] + right[0] * monster_flash_offset[flash_number][1];
	origin[1] = self->current.origin[1] + forward[1] * monster_flash_offset[flash_number][0] + right[1] * monster_flash_offset[flash_number][1];
	origin[2] = self->current.origin[2] + forward[2] * monster_flash_offset[flash_number][0] + right[2] * monster_flash_offset[flash_number][1] + monster_flash_offset[flash_number][2];

	dl = CL_AllocDlight( ent );
	VectorCopy( origin, dl->origin );
	dl->radius = (float)( 200 + ( rand() & 31 ) );
	dl->minlight = 32;
	dl->die = (float)cgi.time();	// + 0.1;

	switch ( flash_number )
	{
	case MZ2_INFANTRY_MACHINEGUN_1:
	case MZ2_INFANTRY_MACHINEGUN_2:
	case MZ2_INFANTRY_MACHINEGUN_3:
	case MZ2_INFANTRY_MACHINEGUN_4:
	case MZ2_INFANTRY_MACHINEGUN_5:
	case MZ2_INFANTRY_MACHINEGUN_6:
	case MZ2_INFANTRY_MACHINEGUN_7:
	case MZ2_INFANTRY_MACHINEGUN_8:
	case MZ2_INFANTRY_MACHINEGUN_9:
	case MZ2_INFANTRY_MACHINEGUN_10:
	case MZ2_INFANTRY_MACHINEGUN_11:
	case MZ2_INFANTRY_MACHINEGUN_12:
	case MZ2_INFANTRY_MACHINEGUN_13:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		CL_ParticleEffect( origin, vec3_origin, 0, 40 );
		CL_SmokeAndFlash( origin );
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "infantry/infatck1.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_SOLDIER_MACHINEGUN_1:
	case MZ2_SOLDIER_MACHINEGUN_2:
	case MZ2_SOLDIER_MACHINEGUN_3:
	case MZ2_SOLDIER_MACHINEGUN_4:
	case MZ2_SOLDIER_MACHINEGUN_5:
	case MZ2_SOLDIER_MACHINEGUN_6:
	case MZ2_SOLDIER_MACHINEGUN_7:
	case MZ2_SOLDIER_MACHINEGUN_8:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		CL_ParticleEffect( origin, vec3_origin, 0, 40 );
		CL_SmokeAndFlash( origin );
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "soldier/solatck3.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_GUNNER_MACHINEGUN_1:
	case MZ2_GUNNER_MACHINEGUN_2:
	case MZ2_GUNNER_MACHINEGUN_3:
	case MZ2_GUNNER_MACHINEGUN_4:
	case MZ2_GUNNER_MACHINEGUN_5:
	case MZ2_GUNNER_MACHINEGUN_6:
	case MZ2_GUNNER_MACHINEGUN_7:
	case MZ2_GUNNER_MACHINEGUN_8:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		CL_ParticleEffect( origin, vec3_origin, 0, 40 );
		CL_SmokeAndFlash( origin );
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "gunner/gunatck2.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_ACTOR_MACHINEGUN_1:
	case MZ2_SUPERTANK_MACHINEGUN_1:
	case MZ2_SUPERTANK_MACHINEGUN_2:
	case MZ2_SUPERTANK_MACHINEGUN_3:
	case MZ2_SUPERTANK_MACHINEGUN_4:
	case MZ2_SUPERTANK_MACHINEGUN_5:
	case MZ2_SUPERTANK_MACHINEGUN_6:
	case MZ2_TURRET_MACHINEGUN:			// PGM
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;

		CL_ParticleEffect( origin, vec3_origin, 0, 40 );
		CL_SmokeAndFlash( origin );
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "infantry/infatck1.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_BOSS2_MACHINEGUN_L1:
	case MZ2_BOSS2_MACHINEGUN_L2:
	case MZ2_BOSS2_MACHINEGUN_L3:
	case MZ2_BOSS2_MACHINEGUN_L4:
	case MZ2_BOSS2_MACHINEGUN_L5:
	case MZ2_CARRIER_MACHINEGUN_L1:		// PMM
	case MZ2_CARRIER_MACHINEGUN_L2:		// PMM
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;

		CL_ParticleEffect( origin, vec3_origin, 0, 40 );
		CL_SmokeAndFlash( origin );
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "infantry/infatck1.wav" ), 1, ATTN_NONE, 0 );
		break;

	case MZ2_SOLDIER_BLASTER_1:
	case MZ2_SOLDIER_BLASTER_2:
	case MZ2_SOLDIER_BLASTER_3:
	case MZ2_SOLDIER_BLASTER_4:
	case MZ2_SOLDIER_BLASTER_5:
	case MZ2_SOLDIER_BLASTER_6:
	case MZ2_SOLDIER_BLASTER_7:
	case MZ2_SOLDIER_BLASTER_8:
	case MZ2_TURRET_BLASTER:			// PGM
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "soldier/solatck2.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_FLYER_BLASTER_1:
	case MZ2_FLYER_BLASTER_2:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "flyer/flyatck3.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_MEDIC_BLASTER_1:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "medic/medatck1.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_HOVER_BLASTER_1:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "hover/hovatck1.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_FLOAT_BLASTER_1:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "floater/fltatck1.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_SOLDIER_SHOTGUN_1:
	case MZ2_SOLDIER_SHOTGUN_2:
	case MZ2_SOLDIER_SHOTGUN_3:
	case MZ2_SOLDIER_SHOTGUN_4:
	case MZ2_SOLDIER_SHOTGUN_5:
	case MZ2_SOLDIER_SHOTGUN_6:
	case MZ2_SOLDIER_SHOTGUN_7:
	case MZ2_SOLDIER_SHOTGUN_8:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		CL_SmokeAndFlash( origin );
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "soldier/solatck1.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_TANK_BLASTER_1:
	case MZ2_TANK_BLASTER_2:
	case MZ2_TANK_BLASTER_3:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "tank/tnkatck3.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_TANK_MACHINEGUN_1:
	case MZ2_TANK_MACHINEGUN_2:
	case MZ2_TANK_MACHINEGUN_3:
	case MZ2_TANK_MACHINEGUN_4:
	case MZ2_TANK_MACHINEGUN_5:
	case MZ2_TANK_MACHINEGUN_6:
	case MZ2_TANK_MACHINEGUN_7:
	case MZ2_TANK_MACHINEGUN_8:
	case MZ2_TANK_MACHINEGUN_9:
	case MZ2_TANK_MACHINEGUN_10:
	case MZ2_TANK_MACHINEGUN_11:
	case MZ2_TANK_MACHINEGUN_12:
	case MZ2_TANK_MACHINEGUN_13:
	case MZ2_TANK_MACHINEGUN_14:
	case MZ2_TANK_MACHINEGUN_15:
	case MZ2_TANK_MACHINEGUN_16:
	case MZ2_TANK_MACHINEGUN_17:
	case MZ2_TANK_MACHINEGUN_18:
	case MZ2_TANK_MACHINEGUN_19:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		CL_ParticleEffect( origin, vec3_origin, 0, 40 );
		CL_SmokeAndFlash( origin );
		Q_sprintf_s( soundname, "tank/tnkatk2%c.wav", 'a' + rand() % 5 );
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( soundname ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_CHICK_ROCKET_1:
	case MZ2_TURRET_ROCKET:			// PGM
		dl->color[0] = 1; dl->color[1] = 0.5; dl->color[2] = 0.2;
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "chick/chkatck2.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_TANK_ROCKET_1:
	case MZ2_TANK_ROCKET_2:
	case MZ2_TANK_ROCKET_3:
		dl->color[0] = 1; dl->color[1] = 0.5; dl->color[2] = 0.2;
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "tank/tnkatck1.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_SUPERTANK_ROCKET_1:
	case MZ2_SUPERTANK_ROCKET_2:
	case MZ2_SUPERTANK_ROCKET_3:
	case MZ2_BOSS2_ROCKET_1:
	case MZ2_BOSS2_ROCKET_2:
	case MZ2_BOSS2_ROCKET_3:
	case MZ2_BOSS2_ROCKET_4:
	case MZ2_CARRIER_ROCKET_1:
//	case MZ2_CARRIER_ROCKET_2:
//	case MZ2_CARRIER_ROCKET_3:
//	case MZ2_CARRIER_ROCKET_4:
		dl->color[0] = 1; dl->color[1] = 0.5; dl->color[2] = 0.2;
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "tank/rocket.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_GUNNER_GRENADE_1:
	case MZ2_GUNNER_GRENADE_2:
	case MZ2_GUNNER_GRENADE_3:
	case MZ2_GUNNER_GRENADE_4:
		dl->color[0] = 1; dl->color[1] = 0.5; dl->color[2] = 0;
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "gunner/gunatck3.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_GLADIATOR_RAILGUN_1:
	// PMM
	case MZ2_CARRIER_RAILGUN:
	case MZ2_WIDOW_RAIL:
	// pmm
		dl->color[0] = 0.5; dl->color[1] = 0.5; dl->color[2] = 1.0;
		break;

// --- Xian's shit starts ---
	case MZ2_MAKRON_BFG:
		dl->color[0] = 0.5; dl->color[1] = 1; dl->color[2] = 0.5;
		//cgi.StartSound (NULL, ent, CHAN_WEAPON, cgi.RegisterSound("makron/bfg_fire.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_MAKRON_BLASTER_1:
	case MZ2_MAKRON_BLASTER_2:
	case MZ2_MAKRON_BLASTER_3:
	case MZ2_MAKRON_BLASTER_4:
	case MZ2_MAKRON_BLASTER_5:
	case MZ2_MAKRON_BLASTER_6:
	case MZ2_MAKRON_BLASTER_7:
	case MZ2_MAKRON_BLASTER_8:
	case MZ2_MAKRON_BLASTER_9:
	case MZ2_MAKRON_BLASTER_10:
	case MZ2_MAKRON_BLASTER_11:
	case MZ2_MAKRON_BLASTER_12:
	case MZ2_MAKRON_BLASTER_13:
	case MZ2_MAKRON_BLASTER_14:
	case MZ2_MAKRON_BLASTER_15:
	case MZ2_MAKRON_BLASTER_16:
	case MZ2_MAKRON_BLASTER_17:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "makron/blaster.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_JORG_MACHINEGUN_L1:
	case MZ2_JORG_MACHINEGUN_L2:
	case MZ2_JORG_MACHINEGUN_L3:
	case MZ2_JORG_MACHINEGUN_L4:
	case MZ2_JORG_MACHINEGUN_L5:
	case MZ2_JORG_MACHINEGUN_L6:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		CL_ParticleEffect( origin, vec3_origin, 0, 40 );
		CL_SmokeAndFlash( origin );
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "boss3/xfire.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_JORG_MACHINEGUN_R1:
	case MZ2_JORG_MACHINEGUN_R2:
	case MZ2_JORG_MACHINEGUN_R3:
	case MZ2_JORG_MACHINEGUN_R4:
	case MZ2_JORG_MACHINEGUN_R5:
	case MZ2_JORG_MACHINEGUN_R6:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		CL_ParticleEffect( origin, vec3_origin, 0, 40 );
		CL_SmokeAndFlash( origin );
		break;

	case MZ2_JORG_BFG_1:
		dl->color[0] = 0.5; dl->color[1] = 1; dl->color[2] = 0.5;
		break;

	case MZ2_BOSS2_MACHINEGUN_R1:
	case MZ2_BOSS2_MACHINEGUN_R2:
	case MZ2_BOSS2_MACHINEGUN_R3:
	case MZ2_BOSS2_MACHINEGUN_R4:
	case MZ2_BOSS2_MACHINEGUN_R5:
	case MZ2_CARRIER_MACHINEGUN_R1:			// PMM
	case MZ2_CARRIER_MACHINEGUN_R2:			// PMM

		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;

		CL_ParticleEffect( origin, vec3_origin, 0, 40 );
		CL_SmokeAndFlash( origin );
		break;

// ======
// ROGUE
	case MZ2_STALKER_BLASTER:
	case MZ2_DAEDALUS_BLASTER:
	case MZ2_MEDIC_BLASTER_2:
	case MZ2_WIDOW_BLASTER:
	case MZ2_WIDOW_BLASTER_SWEEP1:
	case MZ2_WIDOW_BLASTER_SWEEP2:
	case MZ2_WIDOW_BLASTER_SWEEP3:
	case MZ2_WIDOW_BLASTER_SWEEP4:
	case MZ2_WIDOW_BLASTER_SWEEP5:
	case MZ2_WIDOW_BLASTER_SWEEP6:
	case MZ2_WIDOW_BLASTER_SWEEP7:
	case MZ2_WIDOW_BLASTER_SWEEP8:
	case MZ2_WIDOW_BLASTER_SWEEP9:
	case MZ2_WIDOW_BLASTER_100:
	case MZ2_WIDOW_BLASTER_90:
	case MZ2_WIDOW_BLASTER_80:
	case MZ2_WIDOW_BLASTER_70:
	case MZ2_WIDOW_BLASTER_60:
	case MZ2_WIDOW_BLASTER_50:
	case MZ2_WIDOW_BLASTER_40:
	case MZ2_WIDOW_BLASTER_30:
	case MZ2_WIDOW_BLASTER_20:
	case MZ2_WIDOW_BLASTER_10:
	case MZ2_WIDOW_BLASTER_0:
	case MZ2_WIDOW_BLASTER_10L:
	case MZ2_WIDOW_BLASTER_20L:
	case MZ2_WIDOW_BLASTER_30L:
	case MZ2_WIDOW_BLASTER_40L:
	case MZ2_WIDOW_BLASTER_50L:
	case MZ2_WIDOW_BLASTER_60L:
	case MZ2_WIDOW_BLASTER_70L:
	case MZ2_WIDOW_RUN_1:
	case MZ2_WIDOW_RUN_2:
	case MZ2_WIDOW_RUN_3:
	case MZ2_WIDOW_RUN_4:
	case MZ2_WIDOW_RUN_5:
	case MZ2_WIDOW_RUN_6:
	case MZ2_WIDOW_RUN_7:
	case MZ2_WIDOW_RUN_8:
		dl->color[0] = 0; dl->color[1] = 1; dl->color[2] = 0;
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "tank/tnkatck3.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_WIDOW_DISRUPTOR:
		dl->color[0] = -1; dl->color[1] = -1; dl->color[2] = -1;
		cgi.StartSound( NULL, ent, CHAN_WEAPON, cgi.RegisterSound( "weapons/disint2.wav" ), 1, ATTN_NORM, 0 );
		break;

	case MZ2_WIDOW_PLASMABEAM:
	case MZ2_WIDOW2_BEAMER_1:
	case MZ2_WIDOW2_BEAMER_2:
	case MZ2_WIDOW2_BEAMER_3:
	case MZ2_WIDOW2_BEAMER_4:
	case MZ2_WIDOW2_BEAMER_5:
	case MZ2_WIDOW2_BEAM_SWEEP_1:
	case MZ2_WIDOW2_BEAM_SWEEP_2:
	case MZ2_WIDOW2_BEAM_SWEEP_3:
	case MZ2_WIDOW2_BEAM_SWEEP_4:
	case MZ2_WIDOW2_BEAM_SWEEP_5:
	case MZ2_WIDOW2_BEAM_SWEEP_6:
	case MZ2_WIDOW2_BEAM_SWEEP_7:
	case MZ2_WIDOW2_BEAM_SWEEP_8:
	case MZ2_WIDOW2_BEAM_SWEEP_9:
	case MZ2_WIDOW2_BEAM_SWEEP_10:
	case MZ2_WIDOW2_BEAM_SWEEP_11:
		dl->radius = (float)( 300 + ( rand() & 100 ) );
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		dl->die = (float)( cgi.time() + 200 );
		break;
// ROGUE
// ======

// --- Xian's shit ends ---

	}
}

/*
===================
CL_AddDLights
===================
*/
void CL_AddDLights()
{
	int i;
	cdlight_t *dl;

	dl = cl_dlights;

	for ( i = 0; i < MAX_DLIGHTS; i++, dl++ )
	{
		if ( !dl->radius )
		{
			continue;
		}
		cgi.AddLight( dl->origin, dl->radius,
			dl->color[0], dl->color[1], dl->color[2] );
	}
}

/*
===============================================================================

	Particle management

===============================================================================
*/

cparticle_t		*active_particles, *free_particles;

// SlartTodo: Make this cl_particles
cparticle_t		particles[MAX_PARTICLES];

/*
===================
CL_ClearParticles
===================
*/
static void CL_ClearParticles()
{
	int i;

	free_particles = &particles[0];
	active_particles = NULL;

	for ( i = 0; i < MAX_PARTICLES; i++ )
	{
		particles[i].next = &particles[i + 1];
	}
	particles[MAX_PARTICLES - 1].next = NULL;
}

/*
===================
CL_ParticleEffect

Wall impact puffs
===================
*/
void CL_ParticleEffect( vec3_t org, vec3_t dir, int color, int count )
{
	int i, j;
	cparticle_t *p;
	float d;

	for ( i = 0; i < count; i++ )
	{
		if ( !free_particles )
		{
			return;
		}
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		p->color = (float)( color + ( rand() & 7 ) );

		d = (float)( rand() & 31 );
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + ( ( rand() & 7 ) - 4 ) + d * dir[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0f;

		p->alphavel = -1.0f / ( 0.5f + frand() * 0.3f );
	}
}

/*
===================
CL_ParticleEffect2
===================
*/
void CL_ParticleEffect2( vec3_t org, vec3_t dir, int color, int count )
{
	int i, j;
	cparticle_t *p;
	float d;

	for ( i = 0; i < count; i++ )
	{
		if ( !free_particles )
		{
			return;
		}
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		p->color = (float)color;

		d = rand() & 7;
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + ( ( rand() & 7 ) - 4 ) + d * dir[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0f;

		p->alphavel = -1.0f / ( 0.5f + frand() * 0.3f );
	}
}

// RAFAEL
/*
===================
CL_ParticleEffect3
===================
*/
void CL_ParticleEffect3( vec3_t org, vec3_t dir, int color, int count )
{
	int i, j;
	cparticle_t *p;
	float d;

	for ( i = 0; i < count; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		p->color = (float)color;

		d = (float)( rand() & 7 );
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + ( ( rand() & 7 ) - 4 ) + d * dir[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = PARTICLE_GRAVITY;
		p->alpha = 1.0f;

		p->alphavel = -1.0f / ( 0.5f + frand() * 0.3f );
	}
}

/*
===================
CL_TeleporterParticles
===================
*/
void CL_TeleporterParticles( entity_state_t *ent )
{
	int i, j;
	cparticle_t *p;

	for ( i = 0; i < 8; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		p->color = 0xdb;

		for ( j = 0; j < 2; j++ )
		{
			p->org[j] = ent->origin[j] - 16 + ( rand() & 31 );
			p->vel[j] = crand() * 14;
		}

		p->org[2] = ent->origin[2] - 8 + ( rand() & 7 );
		p->vel[2] = 80 + ( rand() & 7 );

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0f;

		p->alphavel = -0.5f;
	}
}

/*
===================
CL_LogoutEffect
===================
*/
void CL_LogoutEffect( vec3_t org, int type )
{
	int i, j;
	cparticle_t *p;

	for ( i = 0; i < 500; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();

		if ( type == MZ_LOGIN )
			p->color = (float)( 0xd0 + ( rand() & 7 ) );	// green
		else if ( type == MZ_LOGOUT )
			p->color = (float)( 0x40 + ( rand() & 7 ) );	// red
		else
			p->color = (float)( 0xe0 + ( rand() & 7 ) );	// yellow

		p->org[0] = org[0] - 16 + frand() * 32;
		p->org[1] = org[1] - 16 + frand() * 32;
		p->org[2] = org[2] - 24 + frand() * 56;

		for ( j = 0; j < 3; j++ )
			p->vel[j] = crand() * 20;

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0f;

		p->alphavel = -1.0f / ( 1.0f + frand() * 0.3f );
	}
}

/*
===================
CL_ItemRespawnParticles
===================
*/
void CL_ItemRespawnParticles( vec3_t org )
{
	int i, j;
	cparticle_t *p;

	for ( i = 0; i < 64; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();

		p->color = 0xd4 + ( rand() & 3 );	// green

		p->org[0] = org[0] + crand() * 8;
		p->org[1] = org[1] + crand() * 8;
		p->org[2] = org[2] + crand() * 8;

		for ( j = 0; j < 3; j++ )
			p->vel[j] = crand() * 8;

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY * 0.2f;
		p->alpha = 1.0f;

		p->alphavel = -1.0f / ( 1.0f + frand() * 0.3f );
	}
}

/*
===================
CL_ExplosionParticles
===================
*/
void CL_ExplosionParticles( vec3_t org )
{
	int i, j;
	cparticle_t *p;

	for ( i = 0; i < 256; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		p->color = (float)( 0xe0 + ( rand() & 7 ) );

		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + ( ( rand() % 32 ) - 16 );
			p->vel[j] = (float)( ( rand() % 384 ) - 192 );
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0f;

		p->alphavel = -0.8f / ( 0.5f + frand() * 0.3f );
	}
}

/*
===================
CL_BigTeleportParticles
===================
*/
void CL_BigTeleportParticles( vec3_t org )
{
	int i;
	cparticle_t *p;
	float angle, dist;
	static int colortable[4] = { 2 * 8, 13 * 8, 21 * 8, 18 * 8 };

	for ( i = 0; i < 4096; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();

		p->color = colortable[rand() & 3];

		angle = M_PI_F * 2 * ( rand() & 1023 ) / 1023.0f;
		dist = (float)( rand() & 31 );
		p->org[0] = org[0] + cosf( angle ) * dist;
		p->vel[0] = cosf( angle ) * ( 70 + ( rand() & 63 ) );
		p->accel[0] = -cosf( angle ) * 100;

		p->org[1] = org[1] + sinf( angle ) * dist;
		p->vel[1] = sinf( angle ) * ( 70 + ( rand() & 63 ) );
		p->accel[1] = -sinf( angle ) * 100;

		p->org[2] = org[2] + 8 + ( rand() % 90 );
		p->vel[2] = -100 + ( rand() & 31 );
		p->accel[2] = PARTICLE_GRAVITY * 4;
		p->alpha = 1.0f;

		p->alphavel = -0.3f / ( 0.5f + frand() * 0.3f );
	}
}

/*
===================
CL_BlasterParticles

Wall impact puffs
===================
*/
void CL_BlasterParticles( vec3_t org, vec3_t dir )
{
	int i, j;
	cparticle_t *p;
	float d;
	int count;

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
		p->color = 0xe0 + ( rand() & 7 );

		d = rand() & 15;
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
===================
CL_BlasterTrail
===================
*/
void CL_BlasterTrail( vec3_t start, vec3_t end )
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
		p->color = 0xe0;
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = move[j] + crand();
			p->vel[j] = crand() * 5;
			p->accel[j] = 0;
		}

		VectorAdd( move, vec, move );
	}
}

/*
===============
CL_QuadTrail

===============
*/
void CL_QuadTrail( vec3_t start, vec3_t end )
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
		p->alphavel = -1.0f / ( 0.8f + frand() * 0.2f );
		p->color = 115;
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
CL_FlagTrail
===================
*/
void CL_FlagTrail( vec3_t start, vec3_t end, float color )
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
CL_DiminishingTrail
===================
*/
void CL_DiminishingTrail( vec3_t start, vec3_t end, centity_t *old, int flags )
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t *p;
	float		dec;
	float		orgscale;
	float		velscale;

	VectorCopy( start, move );
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	dec = 0.5;
	VectorScale( vec, dec, vec );

	if ( old->trailcount > 900 )
	{
		orgscale = 4;
		velscale = 15;
	}
	else if ( old->trailcount > 800 )
	{
		orgscale = 2;
		velscale = 10;
	}
	else
	{
		orgscale = 1;
		velscale = 5;
	}

	while ( len > 0 )
	{
		len -= dec;

		if ( !free_particles )
			return;

		// drop less particles as it flies
		if ( ( rand() & 1023 ) < old->trailcount )
		{
			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;
			VectorClear( p->accel );

			p->time = (float)cgi.time();

			if ( flags & EF_GIB )
			{
				p->alpha = 1.0f;
				p->alphavel = -1.0f / ( 1 + frand() * 0.4f );
				p->color = 0xe8 + ( rand() & 7 );
				for ( j = 0; j < 3; j++ )
				{
					p->org[j] = move[j] + crand() * orgscale;
					p->vel[j] = crand() * velscale;
					p->accel[j] = 0;
				}
				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else if ( flags & EF_GREENGIB )
			{
				p->alpha = 1.0f;
				p->alphavel = -1.0f / ( 1 + frand() * 0.4f );
				p->color = 0xdb + ( rand() & 7 );
				for ( j = 0; j < 3; j++ )
				{
					p->org[j] = move[j] + crand() * orgscale;
					p->vel[j] = crand() * velscale;
					p->accel[j] = 0;
				}
				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else
			{
				p->alpha = 1.0f;
				p->alphavel = -1.0f / ( 1 + frand() * 0.2f );
				p->color = 4 + ( rand() & 7 );
				for ( j = 0; j < 3; j++ )
				{
					p->org[j] = move[j] + crand() * orgscale;
					p->vel[j] = crand() * velscale;
				}
				p->accel[2] = 20;
			}
		}

		old->trailcount -= 5;
		if ( old->trailcount < 100 )
			old->trailcount = 100;
		VectorAdd( move, vec, move );
	}
}

/*
===================
MakeNormalVectors
===================
*/
void MakeNormalVectors( vec3_t forward, vec3_t right, vec3_t up )
{
	float d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct( right, forward );
	VectorMA( right, -d, forward, right );
	VectorNormalize( right );
	CrossProduct( right, forward, up );
}

/*
===================
CL_RocketTrail
===================
*/
void CL_RocketTrail( vec3_t start, vec3_t end, centity_t *old )
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t *p;
	float		dec;

	// smoke
	CL_DiminishingTrail( start, end, old, EF_ROCKET );

	// fire
	VectorCopy( start, move );
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	dec = 1;
	VectorScale( vec, dec, vec );

	while ( len > 0 )
	{
		len -= dec;

		if ( !free_particles )
			return;

		if ( ( rand() & 7 ) == 0 )
		{
			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;

			VectorClear( p->accel );
			p->time = (float)cgi.time();

			p->alpha = 1.0f;
			p->alphavel = -1.0f / ( 1 + frand() * 0.2f );
			p->color = (float)( 0xdc + ( rand() & 3 ) );
			for ( j = 0; j < 3; j++ )
			{
				p->org[j] = move[j] + crand() * 5;
				p->vel[j] = crand() * 20;
			}
			p->accel[2] = -PARTICLE_GRAVITY;
		}
		VectorAdd( move, vec, move );
	}
}

/*
===================
CL_RailTrail
===================
*/
void CL_RailTrail( vec3_t start, vec3_t end )
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t *p;
	float		dec;
	vec3_t		right, up;
	int			i;
	float		d, c, s;
	vec3_t		dir;
	byte		clr = 0x74;

	VectorCopy( start, move );
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	MakeNormalVectors( vec, right, up );

	for ( i = 0; i < len; i++ )
	{
		if ( !free_particles )
			return;

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		VectorClear( p->accel );

		d = i * 0.1f;
		c = cosf( d );
		s = sinf( d );

		VectorScale( right, c, dir );
		VectorMA( dir, s, up, dir );

		p->alpha = 1.0f;
		p->alphavel = -1.0f / ( 1 + frand() * 0.2f );
		p->color = (float)( clr + ( rand() & 7 ) );
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = move[j] + dir[j] * 3;
			p->vel[j] = dir[j] * 6;
		}

		VectorAdd( move, vec, move );
	}

	dec = 0.75f;
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

		p->alpha = 1.0f;
		p->alphavel = -1.0f / ( 0.6f + frand() * 0.2f );
		p->color = (float)( 0x0 + rand() & 15 );

		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = move[j] + crand() * 3;
			p->vel[j] = crand() * 3;
			p->accel[j] = 0;
		}

		VectorAdd( move, vec, move );
	}
}

// RAFAEL
/*
===================
CL_IonripperTrail
===================
*/
void CL_IonripperTrail( vec3_t start, vec3_t ent )
{
	vec3_t	move;
	vec3_t	vec;
	float	len;
	int		j;
	cparticle_t *p;
	int		dec;
	int     left = 0;

	VectorCopy( start, move );
	VectorSubtract( ent, start, vec );
	len = VectorNormalize( vec );

	dec = 5;
	VectorScale( vec, 5, vec );

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
		p->alpha = 0.5f;
		p->alphavel = -1.0f / ( 0.3f + frand() * 0.2f );
		p->color = (float)( 0xe4 + ( rand() & 3 ) );

		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = move[j];
			p->accel[j] = 0;
		}
		if ( left )
		{
			left = 0;
			p->vel[0] = 10;
		}
		else
		{
			left = 1;
			p->vel[0] = -10;
		}

		p->vel[1] = 0;
		p->vel[2] = 0;

		VectorAdd( move, vec, move );
	}
}

/*
===================
CL_BubbleTrail
===================
*/
void CL_BubbleTrail( vec3_t start, vec3_t end )
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

	dec = 32;
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
		p->alphavel = -1.0f / ( 1 + frand() * 0.2f );
		p->color = 4 + ( rand() & 7 );
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = move[j] + crand() * 2;
			p->vel[j] = crand() * 5;
		}
		p->vel[2] += 6;

		VectorAdd( move, vec, move );
	}
}

/*
===================
CL_FlyParticles
===================
*/
void CL_FlyParticles( vec3_t origin, int count )
{
	int			i;
	cparticle_t *p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64;
	float		ltime;

	if ( count > NUMVERTEXNORMALS )
		count = NUMVERTEXNORMALS;

	// Build the table
	if ( !avelocities[0][0] )
	{
		for ( i = 0; i < NUMVERTEXNORMALS * 3; i++ )
			avelocities[0][i] = ( rand() & 255 ) * 0.01f;
	}

	ltime = (float)cgi.time() / 1000.0f;
	for ( i = 0; i < count; i += 2 )
	{
		angle = ltime * avelocities[i][0];
		sy = sinf( angle );
		cy = cosf( angle );
		angle = ltime * avelocities[i][1];
		sp = sinf( angle );
		cp = cosf( angle );
		angle = ltime * avelocities[i][2];
		sr = sinf( angle );
		cr = cosf( angle );

		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;

		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();

		dist = sinf( ltime + i ) * 64;
		p->org[0] = origin[0] + cgi.bytedirs[i][0] * dist + forward[0] * BEAMLENGTH;
		p->org[1] = origin[1] + cgi.bytedirs[i][1] * dist + forward[1] * BEAMLENGTH;
		p->org[2] = origin[2] + cgi.bytedirs[i][2] * dist + forward[2] * BEAMLENGTH;

		VectorClear( p->vel );
		VectorClear( p->accel );

		p->color = 0;
		p->colorvel = 0;

		p->alpha = 1;
		p->alphavel = -100;
	}
}

/*
===================
CL_FlyEffect
===================
*/
void CL_FlyEffect( centity_t *ent, vec3_t origin )
{
	int		n;
	int		count;
	int		starttime;

	if ( ent->fly_stoptime < cgi.time() )
	{
		starttime = cgi.time();
		ent->fly_stoptime = cgi.time() + 60000;
	}
	else
	{
		starttime = ent->fly_stoptime - 60000;
	}

	n = cgi.time() - starttime;
	if ( n < 20000 )
		count = n * 162 / 20000.0f;
	else
	{
		n = ent->fly_stoptime - cgi.time();
		if ( n < 20000 )
			count = n * 162 / 20000.0f;
		else
			count = 162;
	}

	CL_FlyParticles( origin, count );
}

/*
===================
CL_BfgParticles
===================
*/
void CL_BfgParticles( entity_t *ent )
{
	int			i;
	cparticle_t *p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64;
	vec3_t		v;
	float		ltime;

	if ( !avelocities[0][0] )
	{
		for ( i = 0; i < NUMVERTEXNORMALS * 3; i++ )
			avelocities[0][i] = ( rand() & 255 ) * 0.01f;
	}

	ltime = (float)cgi.time() / 1000.0f;
	for ( i = 0; i < NUMVERTEXNORMALS; i++ )
	{
		angle = ltime * avelocities[i][0];
		sy = sinf( angle );
		cy = cosf( angle );
		angle = ltime * avelocities[i][1];
		sp = sinf( angle );
		cp = cosf( angle );
		angle = ltime * avelocities[i][2];
		sr = sinf( angle );
		cr = cosf( angle );

		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;

		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();

		dist = sinf( ltime + i ) * 64;
		p->org[0] = ent->origin[0] + cgi.bytedirs[i][0] * dist + forward[0] * BEAMLENGTH;
		p->org[1] = ent->origin[1] + cgi.bytedirs[i][1] * dist + forward[1] * BEAMLENGTH;
		p->org[2] = ent->origin[2] + cgi.bytedirs[i][2] * dist + forward[2] * BEAMLENGTH;

		VectorClear( p->vel );
		VectorClear( p->accel );

		VectorSubtract( p->org, ent->origin, v );
		dist = VectorLength( v ) / 90.0f;
		p->color = floor( 0xd0 + dist * 7 );
		p->colorvel = 0;

		p->alpha = 1.0f - dist;
		p->alphavel = -100;
	}
}

/*
===================
CL_TrapParticles

Hey if you're reading this, know that this code sucks
===================
*/
// RAFAEL
void CL_TrapParticles( entity_t *ent )
{
	vec3_t		move;
	vec3_t		vec;
	vec3_t		start, end;
	float		len;
	int			j;
	cparticle_t *p;
	int			dec;

	ent->origin[2] -= 14;
	VectorCopy( ent->origin, start );
	VectorCopy( ent->origin, end );
	end[2] += 64;

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
		p->color = 0xe0;
		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = move[j] + crand();
			p->vel[j] = crand() * 15;
			p->accel[j] = 0;
		}
		p->accel[2] = PARTICLE_GRAVITY;

		VectorAdd( move, vec, move );
	}

	{
		int			i, j, k;
		cparticle_t *p;
		float		vel;
		vec3_t		dir;
		vec3_t		org;

		ent->origin[2] += 14;
		VectorCopy( ent->origin, org );

		for ( i = -2; i <= 2; i += 4 )
		{
			for ( j = -2; j <= 2; j += 4 )
			{
				for ( k = -2; k <= 4; k += 4 )
				{
					if ( !free_particles )
						return;
					p = free_particles;
					free_particles = p->next;
					p->next = active_particles;
					active_particles = p;

					p->time = (float)cgi.time();
					p->color = 0xe0 + ( rand() & 3 );

					p->alpha = 1.0f;
					p->alphavel = -1.0f / ( 0.3f + ( rand() & 7 ) * 0.02f );

					p->org[0] = org[0] + i + ( ( rand() & 23 ) * crand() );
					p->org[1] = org[1] + j + ( ( rand() & 23 ) * crand() );
					p->org[2] = org[2] + k + ( ( rand() & 23 ) * crand() );

					dir[0] = (float)( j * 8 );
					dir[1] = (float)( i * 8 );
					dir[2] = (float)( k * 8 );

					VectorNormalize( dir );
					vel = (float)( 50 + rand() & 63 );
					VectorScale( dir, vel, p->vel );

					p->accel[0] = p->accel[1] = 0;
					p->accel[2] = -PARTICLE_GRAVITY;
				}
			}
		}
	}
}

/*
===================
CL_BFGExplosionParticles

FIXME combined with CL_ExplosionParticles
===================
*/
void CL_BFGExplosionParticles( vec3_t org )
{
	int i, j;
	cparticle_t *p;

	for ( i = 0; i < 256; i++ )
	{
		if ( !free_particles )
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cgi.time();
		p->color = (float)( 0xd0 + ( rand() & 7 ) );

		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + ( ( rand() % 32 ) - 16 );
			p->vel[j] = ( rand() % 384 ) - 192;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0f;

		p->alphavel = -0.8f / ( 0.5f + frand() * 0.3f );
	}
}

/*
===================
CL_TeleportParticles
===================
*/
void CL_TeleportParticles( vec3_t org )
{
	int			i, j, k;
	cparticle_t *p;
	float		vel;
	vec3_t		dir;

	for ( i = -16; i <= 16; i += 4 )
	{
		for ( j = -16; j <= 16; j += 4 )
		{
			for ( k = -16; k <= 32; k += 4 )
			{
				if ( !free_particles )
					return;
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;

				p->time = (float)cgi.time();
				p->color = (float)( 7 + ( rand() & 7 ) );

				p->alpha = 1.0f;
				p->alphavel = -1.0f / ( 0.3f + ( rand() & 7 ) * 0.02f );

				p->org[0] = org[0] + i + ( rand() & 3 );
				p->org[1] = org[1] + j + ( rand() & 3 );
				p->org[2] = org[2] + k + ( rand() & 3 );

				dir[0] = (float)( j * 8 );
				dir[1] = (float)( i * 8 );
				dir[2] = (float)( k * 8 );

				VectorNormalize( dir );
				vel = (float)( 50 + ( rand() & 63 ) );
				VectorScale( dir, vel, p->vel );

				p->accel[0] = p->accel[1] = 0;
				p->accel[2] = -PARTICLE_GRAVITY;
			}
		}
	}
}

/*
===================
CL_AddParticles
===================
*/
void CL_AddParticles()
{
	cparticle_t		*p, *next;
	float			alpha;
	float			time, time2;
	vec3_t			org;
	int				color;
	cparticle_t		*active, *tail;

	active = NULL;
	tail = NULL;

	for ( p = active_particles; p; p = next )
	{
		next = p->next;

		// PMM - added INSTANT_PARTICLE handling for heat beam
		if ( p->alphavel != INSTANT_PARTICLE )
		{
			time = ( cgi.time() - p->time ) * 0.001f;
			alpha = p->alpha + time * p->alphavel;
			if ( alpha <= 0 )
			{	// faded out
				p->next = free_particles;
				free_particles = p;
				continue;
			}
		}
		else
		{
			time = 0;
			alpha = p->alpha;
		}

		p->next = NULL;
		if ( !tail )
			active = tail = p;
		else
		{
			tail->next = p;
			tail = p;
		}

		if ( alpha > 1.0f )
			alpha = 1.0f;
		color = (float)p->color;

		time2 = time * time;

		org[0] = p->org[0] + p->vel[0] * time + p->accel[0] * time2;
		org[1] = p->org[1] + p->vel[1] * time + p->accel[1] * time2;
		org[2] = p->org[2] + p->vel[2] * time + p->accel[2] * time2;

		cgi.AddParticle( org, color, alpha );
		// PMM
		if ( p->alphavel == INSTANT_PARTICLE )
		{
			p->alphavel = 0.0;
			p->alpha = 0.0;
		}
	}

	active_particles = active;
}

/*
===================
CL_EntityEvent

An entity has just been parsed that has an event value

The female events are there for backwards compatability
===================
*/
void CL_EntityEvent( entity_state_t *ent )
{
	switch ( ent->event )
	{
	case EV_ITEM_RESPAWN:
		cgi.StartSound( NULL, ent->number, CHAN_WEAPON, cgi.RegisterSound( "items/respawn1.wav" ), 1, ATTN_IDLE, 0 );
		CL_ItemRespawnParticles( ent->origin );
		break;
	case EV_PLAYER_TELEPORT:
		cgi.StartSound( NULL, ent->number, CHAN_WEAPON, cgi.RegisterSound( "misc/tele1.wav" ), 1, ATTN_IDLE, 0 );
		CL_TeleportParticles( ent->origin );
		break;
	case EV_FALLSHORT:
		cgi.StartSound( NULL, ent->number, CHAN_AUTO, cgi.RegisterSound( "player/land1.wav" ), 1, ATTN_NORM, 0 );
		break;
	case EV_FALL:
		cgi.StartSound( NULL, ent->number, CHAN_AUTO, cgi.RegisterSound( "*fall2.wav" ), 1, ATTN_NORM, 0 );
		break;
	case EV_FALLFAR:
		cgi.StartSound( NULL, ent->number, CHAN_AUTO, cgi.RegisterSound( "*fall1.wav" ), 1, ATTN_NORM, 0 );
		break;
	}
}

/*
===================
CL_ClearEffects
===================
*/
void CL_ClearEffects()
{
	CL_ClearParticles();
	CL_ClearDlights();
	CL_ClearLightStyles();
}
