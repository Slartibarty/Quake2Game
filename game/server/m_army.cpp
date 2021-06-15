/*
==============================================================================

SOLDIER / PLAYER

==============================================================================
*/

#include "g_local.h"

static int	sound_death;
static int	sound_idle;
static int	sound_pain1;
static int	sound_pain2;
static int	sound_attack;
static int	sound_sight;

void army_idle_think( edict_t *self )
{
	if ( random() < 0.2f )
		gi.sound( self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0 );
}

void army_ondeath( edict_t *self )
{
	VectorSet( self->mins, -16, -16, -24 );
	VectorSet( self->maxs, 16, 16, -8 );
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity( self );
}

extern void army_run( edict_t *self );
extern void army_stand( edict_t *self );
extern void army_fire( edict_t *self );

mframe_t army_frames_stand[8]
{
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL }
};
mmove_t army_move_stand{ 0, 7, army_frames_stand, army_stand };

void army_stand( edict_t *self )
{
	self->monsterinfo.currentmove = &army_move_stand;
}

mframe_t army_frames_death1[10]
{
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL }
};
mmove_t army_move_death1{ 8, 17, army_frames_death1, army_ondeath };

mframe_t army_frames_death2[11]
{
	{ ai_move, 0, NULL },
	{ ai_move, -5, NULL },
	{ ai_move, -4, NULL },
	{ ai_move, -13, NULL },
	{ ai_move, -3, NULL },
	{ ai_move, -4, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL }
};
mmove_t army_move_death2{ 18, 28, army_frames_death2, army_ondeath };

mframe_t army_frames_pain1[6]
{
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 1, NULL }
};
mmove_t army_move_pain1{ 40, 45, army_frames_pain1, army_run };

mframe_t army_frames_pain2[14]
{
	{ ai_move, 0, NULL },
	{ ai_move, 13, NULL },
	{ ai_move, 9, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 2, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL }
};
mmove_t army_move_pain2{ 46, 60, army_frames_pain2, army_run };

mframe_t army_frames_pain3[13]
{
	{ ai_move, 0, NULL },
	{ ai_move, 1, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 1, NULL },
	{ ai_move, 1, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 1, NULL },
	{ ai_move, 4, NULL },
	{ ai_move, 3, NULL },
	{ ai_move, 6, NULL },
	{ ai_move, 8, NULL },
	{ ai_move, 0, NULL }
};
mmove_t army_move_pain3{ 61, 72, army_frames_pain3, army_run };

mframe_t army_frames_run[8]
{
	{ ai_run, 11, army_idle_think },
	{ ai_run, 15, NULL },
	{ ai_run, 10, NULL },
	{ ai_run, 10, NULL },
	{ ai_run, 8, NULL },
	{ ai_run, 15, NULL },
	{ ai_run, 10, NULL },
	{ ai_run, 8, NULL }
};
mmove_t army_move_run{ 73, 80, army_frames_run, army_run };

void army_run( edict_t *self )
{
	if ( self->monsterinfo.aiflags & AI_STAND_GROUND )
		self->monsterinfo.currentmove = &army_move_stand;
	else
		self->monsterinfo.currentmove = &army_move_run;
}

mframe_t army_frames_shoot[9]
{
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, army_fire },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL }
};
mmove_t army_move_shoot{ 81, 89, army_frames_shoot, army_run };

void army_attack( edict_t *self )
{
	self->monsterinfo.currentmove = &army_move_shoot;
}

mframe_t army_frames_walk[24]
{
	{ ai_walk, 1, army_idle_think },
	{ ai_walk, 1, NULL },
	{ ai_walk, 1, NULL },
	{ ai_walk, 1, NULL },
	{ ai_walk, 2, NULL },
	{ ai_walk, 3, NULL },
	{ ai_walk, 4, NULL },
	{ ai_walk, 4, NULL },
	{ ai_walk, 2, NULL },
	{ ai_walk, 2, NULL },
	{ ai_walk, 2, NULL },
	{ ai_walk, 1, NULL },
	{ ai_walk, 0, NULL },
	{ ai_walk, 1, NULL },
	{ ai_walk, 1, NULL },
	{ ai_walk, 1, NULL },
	{ ai_walk, 3, NULL },
	{ ai_walk, 3, NULL },
	{ ai_walk, 3, NULL },
	{ ai_walk, 3, NULL },
	{ ai_walk, 2, NULL },
	{ ai_walk, 1, NULL },
	{ ai_walk, 1, NULL },
	{ ai_walk, 1, NULL }
};
mmove_t army_move_walk{ 90, 113, army_frames_walk, NULL };

void army_walk( edict_t *self )
{
	self->monsterinfo.currentmove = &army_move_walk;
}

void army_fire( edict_t *self )
{
	vec3_t	dir;

	// fire somewhat behind the player, so a dodging player is harder to hit

	VectorScale( self->enemy->velocity, 0.2f, dir );
	VectorSubtract( self->enemy->s.origin, dir, dir );
	VectorSubtract( dir, self->s.origin, dir );
	VectorNormalize( dir );

	gi.sound( self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0 );

	fire_shotgun( self, self->s.origin, dir, 4, 0, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, 4, MOD_UNKNOWN );
}

void army_pain( edict_t *self, edict_t *other, float kick, int damage )
{
	float	r;

	if ( level.time < self->pain_debounce_time )
		return;

	r = random();

	if ( r < 0.2f )
	{
		self->pain_debounce_time = level.time + 0.6f;
		self->monsterinfo.currentmove = &army_move_pain1;

		gi.sound( self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0 );
	}
	else if ( r < 0.6f )
	{
		self->pain_debounce_time = level.time + 1.1f;
		self->monsterinfo.currentmove = &army_move_pain2;

		gi.sound( self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0 );
	}
	else
	{
		self->pain_debounce_time = level.time + 1.1f;
		self->monsterinfo.currentmove = &army_move_pain3;
		gi.sound( self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0 );
	}

}

void army_die( edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point )
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

	if ( random() < 0.5f )
		self->monsterinfo.currentmove = &army_move_death1;
	else
		self->monsterinfo.currentmove = &army_move_death2;
}

void army_sight( edict_t *self, edict_t *other )
{
	gi.sound( self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0 );
}

void SP_monster_army( edict_t *self )
{
	if ( deathmatch->GetBool() )
	{
		G_FreeEdict( self );
		return;
	}

	sound_death = gi.soundindex( "soldier/death1.wav" );
	sound_idle = gi.soundindex( "soldier/idle.wav" );
	sound_pain1 = gi.soundindex( "soldier/pain1.wav" );
	sound_pain2 = gi.soundindex( "soldier/pain2.wav" );
	sound_attack = gi.soundindex( "soldier/sattck1.wav" );
	sound_sight = gi.soundindex( "soldier/sight1.wav" );

	gi.soundindex( "misc/udeath.wav" );

	self->s.modelindex = gi.modelindex( "models/soldier/tris.md2" );
	VectorSet( self->mins, -16, -16, -24 );
	VectorSet( self->maxs, 16, 16, 40 );

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->mass = 100;
	self->health = 30;
	self->gib_health = -35;

	self->monsterinfo.stand = army_stand;
	self->monsterinfo.walk = army_walk;
	self->monsterinfo.run = army_run;
	self->monsterinfo.attack = army_attack;
	self->monsterinfo.sight = army_sight;

	self->pain = army_pain;
	self->die = army_die;

	gi.linkentity( self );

	self->monsterinfo.currentmove = &army_move_stand;
	self->monsterinfo.scale = 1.0f;

	walkmonster_start( self );
}
