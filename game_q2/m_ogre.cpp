/*
==============================================================================

OGRE

==============================================================================
*/

#include "g_local.h"

static int	sound_drag;
static int	sound_death;
static int	sound_idle1;
static int	sound_idle2;
static int	sound_pain1;
static int	sound_sawattack;
static int	sound_grenattack;
static int	sound_wake;

void OgreFireGrenade( edict_t *self )
{
	vec3_t	forward;

	AngleVectors( self->s.angles, forward, NULL, NULL );

	gi.sound( self, CHAN_WEAPON, sound_grenattack, 1, ATTN_NORM, 0 );

	fire_grenade( self, self->s.origin, forward, 40, 600, 2.5f, 80 );
}

void chainsaw( edict_t *self )
{
	vec3_t	delta;
	vec3_t	aim;
	float	ldmg;

	if ( !self->enemy )
		return;
	if ( !CanDamage( self->enemy, self ) )
		return;

	VectorSubtract( self->enemy->s.origin, self->s.origin, delta );

	if ( VectorLength( delta ) > 100.0f )
		return;

	ldmg = ( random() + random() + random() ) * 4;

	VectorSet( aim, MELEE_DISTANCE, 0, 0 );

	fire_hit( self, aim, ldmg, 0 );
}

void ogre_idle_think( edict_t *self )
{
	if ( random() < 0.1f )
		gi.sound( self, CHAN_VOICE, sound_idle1, 1, ATTN_IDLE, 0 );
}

mframe_t ogre_frames_stand[9]
{
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, ogre_idle_think },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL },
	{ ai_stand, 0, NULL }
};
mmove_t ogre_move_stand{ 0, 8, ogre_frames_stand, NULL };

void ogre_stand( edict_t *self )
{
	self->monsterinfo.currentmove = &ogre_move_stand;
}

void ogre_walk_think1( edict_t *self )
{
	if ( random() < 0.2f )
		gi.sound( self, CHAN_VOICE, sound_idle1, 1, ATTN_IDLE, 0 );
}

void ogre_walk_think2( edict_t *self )
{
	if ( random() < 0.1f )
		gi.sound( self, CHAN_VOICE, sound_drag, 1, ATTN_IDLE, 0 );
}

mframe_t ogre_frames_walk[16]
{
	{ ai_walk, 3, NULL },
	{ ai_walk, 2, NULL },
	{ ai_walk, 2, ogre_walk_think1 },
	{ ai_walk, 2, NULL },
	{ ai_walk, 2, NULL },
	{ ai_walk, 5, ogre_walk_think2 },
	{ ai_walk, 3, NULL },
	{ ai_walk, 2, NULL },
	{ ai_walk, 3, NULL },
	{ ai_walk, 1, NULL },
	{ ai_walk, 2, NULL },
	{ ai_walk, 3, NULL },
	{ ai_walk, 3, NULL },
	{ ai_walk, 3, NULL },
	{ ai_walk, 3, NULL },
	{ ai_walk, 4, NULL }
};
mmove_t ogre_move_walk{ 9, 24, ogre_frames_walk, NULL };

void ogre_walk( edict_t *self )
{
	self->monsterinfo.currentmove = &ogre_move_walk;
}

void ogre_run_think( edict_t *self )
{
	if ( random() < 0.2f )
		gi.sound( self, CHAN_VOICE, sound_idle2, 1, ATTN_IDLE, 0 );
}

mframe_t ogre_frames_run[8]
{
	{ ai_run, 9, ogre_run_think },
	{ ai_run, 12, NULL },
	{ ai_run, 8, NULL },
	{ ai_run, 22, NULL },
	{ ai_run, 16, NULL },
	{ ai_run, 4, NULL },
	{ ai_run, 13, NULL },
	{ ai_run, 24, NULL }
};
mmove_t ogre_move_run{ 25, 32, ogre_frames_run, NULL };

void ogre_run( edict_t *self )
{
	if ( self->monsterinfo.aiflags & AI_STAND_GROUND )
		self->monsterinfo.currentmove = &ogre_move_stand;
	else
		self->monsterinfo.currentmove = &ogre_move_run;
}

void ogre_saw_think( edict_t *self )
{
	gi.sound( self, CHAN_WEAPON, sound_sawattack, 1, ATTN_NORM, 0 );
}

void ogre_swing_think( edict_t *self )
{
	chainsaw( self );

	self->s.angles[1] += random() * 25;
}

void ogre_smash_think( edict_t *self )
{
	chainsaw( self );

	self->nextthink += random() * 0.2f;
}

mframe_t ogre_frames_swing[14]
{
	{ ai_charge, 11, ogre_saw_think },
	{ ai_charge, 1, NULL },
	{ ai_charge, 4, NULL },
	{ ai_charge, 13, NULL },
	{ ai_charge, 9, ogre_swing_think },
	{ ai_charge, 0, ogre_swing_think },
	{ ai_charge, 0, ogre_swing_think },
	{ ai_charge, 0, ogre_swing_think },
	{ ai_charge, 0, ogre_swing_think },
	{ ai_charge, 0, ogre_swing_think },
	{ ai_charge, 0, ogre_swing_think },
	{ ai_charge, 3, NULL },
	{ ai_charge, 8, NULL },
	{ ai_charge, 9, NULL }
};
mmove_t ogre_move_swing{ 33, 46, ogre_frames_swing, ogre_run };

mframe_t ogre_frames_smash[14]
{
	{ ai_charge, 6, ogre_saw_think },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL },
	{ ai_charge, 1, NULL },
	{ ai_charge, 4, NULL },
	{ ai_charge, 4, chainsaw },
	{ ai_charge, 4, chainsaw },
	{ ai_charge, 10, chainsaw },
	{ ai_charge, 13, chainsaw },
	{ ai_charge, 0, chainsaw },
	{ ai_charge, 2, ogre_smash_think },
	{ ai_charge, 0, NULL },
	{ ai_charge, 4, NULL },
	{ ai_charge, 12, NULL }
};
mmove_t ogre_move_smash{ 47, 60, ogre_frames_smash, ogre_run };

mframe_t ogre_frames_nail[6]
{
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, OgreFireGrenade },
	{ ai_charge, 0, NULL },
	{ ai_charge, 0, NULL }
};
mmove_t ogre_move_nail{ 61, 66, ogre_frames_nail, ogre_run };

void ogre_attack( edict_t *self )
{
	self->monsterinfo.currentmove = &ogre_move_nail;
}

void ogre_melee( edict_t *self )
{
	if ( random() > 0.5f )
	{
		self->monsterinfo.currentmove = &ogre_move_smash;
	}
	else
	{
		self->monsterinfo.currentmove = &ogre_move_swing;
	}
}

mframe_t ogre_frames_pain1[5]
{
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL }
};
mmove_t ogre_move_pain1{ 67, 71, ogre_frames_pain1, ogre_run };

mframe_t ogre_frames_pain2[3]
{
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL }
};
mmove_t ogre_move_pain2{ 72, 74, ogre_frames_pain2, ogre_run };

mframe_t ogre_frames_pain3[6]
{
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL }
};
mmove_t ogre_move_pain3{ 75, 80, ogre_frames_pain3, ogre_run };

mframe_t ogre_frames_pain4[16]
{
	{ ai_move, 0, NULL },
	{ ai_move, 10, NULL },
	{ ai_move, 9, NULL },
	{ ai_move, 4, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
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
mmove_t ogre_move_pain4{ 81, 96, ogre_frames_pain4, ogre_run };

mframe_t ogre_frames_pain5[15]
{
	{ ai_move, 0, NULL },
	{ ai_move, 10, NULL },
	{ ai_move, 9, NULL },
	{ ai_move, 4, NULL },
	{ ai_move, 0, NULL },
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
mmove_t ogre_move_pain5{ 97, 111, ogre_frames_pain5, ogre_run };

// TODO
void ogre_dead( edict_t *self )
{
	VectorSet( self->mins, -16, -16, -24 );
	VectorSet( self->maxs, 16, 16, -8 );
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity( self );
}

mframe_t ogre_frames_die1[14]
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
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL }
};
mmove_t ogre_move_die1{ 112, 125, ogre_frames_die1, ogre_dead };

mframe_t ogre_frames_die2[10]
{
	{ ai_move, 0, NULL },
	{ ai_move, 5, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 1, NULL },
	{ ai_move, 3, NULL },
	{ ai_move, 7, NULL },
	{ ai_move, 25, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL },
	{ ai_move, 0, NULL }
};
mmove_t ogre_move_die2{ 126, 135, ogre_frames_die2, ogre_dead };


void ogre_sight( edict_t *self, edict_t *other )
{
	gi.sound( self, CHAN_VOICE, sound_wake, 1, ATTN_NORM, 0 );
}

void ogre_pain( edict_t *self, edict_t *other, float kick, int damage )
{
	float r;

	if ( level.time < self->pain_debounce_time )
		return;

	gi.sound( self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0 );

	r = random();

	if ( r < 0.25f )
	{
		self->monsterinfo.currentmove = &ogre_move_pain1;
		self->pain_debounce_time = level.time + 1;
	}
	else if ( r < 0.5f )
	{
		self->monsterinfo.currentmove = &ogre_move_pain2;
		self->pain_debounce_time = level.time + 1;
	}
	else if ( r < 0.75f )
	{
		self->monsterinfo.currentmove = &ogre_move_pain3;
		self->pain_debounce_time = level.time + 1;
	}
	else if ( r < 0.88f )
	{
		self->monsterinfo.currentmove = &ogre_move_pain4;
		self->pain_debounce_time = level.time + 2;
	}
	else
	{
		self->monsterinfo.currentmove = &ogre_move_pain5;
		self->pain_debounce_time = level.time + 2;
	}
}

void ogre_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n= 0; n < 4; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowHead (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

// regular death
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	gi.sound( self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0 );

	if ( random() < 0.5f )
		self->monsterinfo.currentmove = &ogre_move_die1;
	else
		self->monsterinfo.currentmove = &ogre_move_die2;
}

//-------------------------------------------------------------------------------------------------

void SP_monster_ogre (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_drag = gi.soundindex( "ogre/ogdrag.wav" );
	sound_death = gi.soundindex( "ogre/ogdth.wav" );
	sound_idle1 = gi.soundindex( "ogre/ogidle.wav" );
	sound_idle2 = gi.soundindex( "ogre/ogidle2.wav" );
	sound_pain1 = gi.soundindex( "ogre/ogpain1.wav" );
	sound_sawattack = gi.soundindex( "ogre/ogsawatk.wav" );
	sound_grenattack = gi.soundindex( "weapons/grenade.wav" );
	sound_wake = gi.soundindex( "ogre/ogwake.wav" );
	
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->s.modelindex = gi.modelindex("models/ogre/tris.md2");
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 32);

	self->health = 200;
	self->gib_health = -80;
	self->mass = 200;

	self->pain = ogre_pain;
	self->die = ogre_die;

	self->monsterinfo.stand = ogre_stand;
	self->monsterinfo.walk = ogre_walk;
	self->monsterinfo.run = ogre_run;
	self->monsterinfo.attack = ogre_attack;
	self->monsterinfo.melee = ogre_melee;
	self->monsterinfo.sight = ogre_sight;

	gi.linkentity (self);

	self->monsterinfo.currentmove = &ogre_move_stand;
	self->monsterinfo.scale = 1.0f;

	walkmonster_start (self);
}
