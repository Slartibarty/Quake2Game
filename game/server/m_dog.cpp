/*
===================================================================================================

DOG

===================================================================================================
*/

#include "g_local.h"

//-------------------------------------------------------------------------------------------------

static int	sound_attack;
static int	sound_death;
static int	sound_pain;
static int	sound_sight;
static int	sound_idle;

//-------------------------------------------------------------------------------------------------

extern void dog_run( edict_t *self );
extern void dog_attack( edict_t *self );

void Dog_JumpTouch( edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf )
{
	if ( self->health <= 0 )
	{
		self->touch = NULL;
		return;
	}

	if ( other->takedamage )
	{
		if ( VectorLength( self->velocity ) )
		{
			vec3_t	point;
			vec3_t	normal;
			int		damage;

			VectorCopy( self->velocity, normal );
			VectorNormalize( normal );
			VectorMA( self->s.origin, self->maxs[0], normal, point );
			damage = int( 10 + 10 * random() );
			T_Damage( other, self, self, self->velocity, point, normal, damage, damage, 0, MOD_UNKNOWN );
		}
	}

	if ( !M_CheckBottom( self ) )
	{
		if ( self->groundentity )
		{
			self->touch = NULL;
			self->nextthink = level.time + FRAMETIME;
			dog_attack( self );
		}
		return; // not on ground yet
	}

	self->touch = NULL;
	self->nextthink = level.time + FRAMETIME;
}

void dog_dead( edict_t *self )
{
	VectorSet( self->mins, -16, -16, -24 );
	VectorSet( self->maxs, 16, 16, -8 );
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity( self );

	M_FlyCheck( self );
}

void dog_bite( edict_t *self )
{
	vec3_t	aim;
	int		ldmg;

	VectorSet( aim, MELEE_DISTANCE, self->mins[0], 8 );

	ldmg = int( ( random() + random() + random() ) * 8 );

	gi.sound( self, CHAN_VOICE, sound_attack, 1, ATTN_NORM, 0 );

	fire_hit( self, aim, ldmg, 40 );
}

void dog_do_idle( edict_t *self )
{
	if ( random() < 0.2f )
		gi.sound( self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0 );
}

void dog_onleap( edict_t *self )
{
	vec3_t	forward;
	AngleVectors( self->s.angles, forward, NULL, NULL );

	// Launch ourselves
	self->s.origin[2] += 1;
	VectorScale( forward, 300, self->velocity );
	self->velocity[2] = 200;
	self->groundentity = NULL;

	self->touch = Dog_JumpTouch;
}

//-------------------------------------------------------------------------------------------------

mframe_t dog_frames_attack[8]
{
	{ ai_charge, 10, NULL },
	{ ai_charge, 10, NULL },
	{ ai_charge, 10, NULL },
	{ ai_charge, 10, dog_bite },
	{ ai_charge, 10, NULL },
	{ ai_charge, 10, NULL },
	{ ai_charge, 10, NULL },
	{ ai_charge, 10, NULL }
};
mmove_t dog_move_attack{ 0, 7, dog_frames_attack, dog_run };

mframe_t dog_frames_death1[9]
{
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL }
};
mmove_t dog_move_death1{ 8, 16, dog_frames_death1, dog_dead };

mframe_t dog_frames_death2[9]
{
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL }
};
mmove_t dog_move_death2{ 17, 25, dog_frames_death2, dog_dead };

mframe_t dog_frames_pain1[6]
{
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL }
};
mmove_t dog_move_pain1{ 26, 31, dog_frames_pain1, dog_run };

mframe_t dog_frames_pain2[16]
{
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 4, NULL },
	{ ai_move, 12, NULL },
	{ ai_move, 12, NULL },
	{ ai_move, 2, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 4, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 10, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL }
};
mmove_t dog_move_pain2{ 32, 47, dog_frames_pain2, dog_run };

mframe_t dog_frames_run[12]
{
	{ ai_run, 16, dog_do_idle },
	{ ai_run, 32, NULL },
	{ ai_run, 32, NULL },
	{ ai_run, 20, NULL },
	{ ai_run, 64, NULL },
	{ ai_run, 32, NULL },
	{ ai_run, 16, NULL },
	{ ai_run, 32, NULL },
	{ ai_run, 32, NULL },
	{ ai_run, 20, NULL },
	{ ai_run, 64, NULL },
	{ ai_run, 32, NULL }
};
mmove_t dog_move_run{ 48, 59, dog_frames_run, NULL };

mframe_t dog_frames_leap[9]
{
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, dog_onleap },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL }
};
mmove_t dog_move_leap{ 60, 68, dog_frames_leap, dog_run };

mframe_t dog_frames_stand[9]
{
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL }
};
mmove_t dog_move_stand{ 69, 77, dog_frames_stand, NULL };

mframe_t dog_frames_walk[8]
{
	{ ai_walk, 8, dog_do_idle },
	{ ai_walk, 8, NULL },
	{ ai_walk, 8, NULL },
	{ ai_walk, 8, NULL },
	{ ai_walk, 8, NULL },
	{ ai_walk, 8, NULL },
	{ ai_walk, 8, NULL },
	{ ai_walk, 8, NULL }
};
mmove_t dog_move_walk{ 78, 85, dog_frames_walk, NULL };

//-------------------------------------------------------------------------------------------------

void dog_stand( edict_t *self )
{
	self->monsterinfo.currentmove = &dog_move_stand;
}

void dog_walk( edict_t *self )
{
	self->monsterinfo.currentmove = &dog_move_walk;
}

void dog_run( edict_t *self )
{
	self->monsterinfo.currentmove = &dog_move_run;
}

void dog_attack( edict_t *self )
{
	self->monsterinfo.currentmove = &dog_move_leap;
}

void dog_melee( edict_t *self )
{
	self->monsterinfo.currentmove = &dog_move_attack;
}

void dog_sight( edict_t *self, edict_t *other )
{
	gi.sound( self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0 );
}

void dog_pain( edict_t *self, edict_t *other, float kick, int damage )
{
	gi.sound( self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0 );

	if ( random() > 0.5f )
		self->monsterinfo.currentmove = &dog_move_pain1;
	else
		self->monsterinfo.currentmove = &dog_move_pain2;
}

void dog_die( edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point )
{
	int		n;

	// check for gib
	if ( self->health <= self->gib_health )
	{
		gi.sound( self, CHAN_VOICE, gi.soundindex( "misc/udeath.wav" ), 1, ATTN_NORM, 0 );
		for ( n = 0; n < 3; n++ )
			ThrowGib( self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC );
		ThrowGib( self, "models/objects/gibs/chest/tris.md2", damage, GIB_ORGANIC );
		ThrowHead( self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC );
		self->deadflag = DEAD_DEAD;
		return;
	}

	if ( self->deadflag == DEAD_DEAD )
		return;

	// regular death
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	gi.sound( self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0 );

	if ( random() > 0.5f )
		self->monsterinfo.currentmove = &dog_move_death1;
	else
		self->monsterinfo.currentmove = &dog_move_death2;
}

//-------------------------------------------------------------------------------------------------

void SP_monster_dog( edict_t *self )
{
	if ( deathmatch->GetBool() )
	{
		G_FreeEdict( self );
		return;
	}

	sound_attack = gi.soundindex( "dog/dattack1.wav" );
	sound_death = gi.soundindex( "dog/ddeath.wav" );
	sound_pain = gi.soundindex( "dog/dpain1.wav" );
	sound_sight = gi.soundindex( "dog/dsight.wav" );
	sound_idle = gi.soundindex( "dog/idle.wav" );

	gi.soundindex( "misc/udeath.wav" );

	self->s.modelindex = gi.modelindex( "models/dog/tris.md2" );
	VectorSet( self->mins, -32, -32, -24 );
	VectorSet( self->maxs, 32, 32, 40 );

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->mass = 65;
	self->health = 25;
	self->gib_health = -35;

	self->monsterinfo.stand = dog_stand;
	self->monsterinfo.walk = dog_walk;
	self->monsterinfo.run = dog_run;
	self->monsterinfo.attack = dog_attack;
	self->monsterinfo.melee = dog_melee;
	self->monsterinfo.sight = dog_sight;

	self->pain = dog_pain;
	self->die = dog_die;

	gi.linkentity( self );

	dog_stand( self );
	self->monsterinfo.scale = 1.0f;

	walkmonster_start( self );
}
