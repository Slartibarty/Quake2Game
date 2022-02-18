
#include "g_local.h"

/*
===================================================================================================

	Physics

===================================================================================================
*/

void Phys_Simulate( float deltaTime )
{
	gi.physSystem->Simulate( deltaTime );

	for ( int i = game.maxclients + 1; i < globals.num_edicts; ++i )
	{
		edict_t *ent = g_edicts + i;

		if ( !ent->bodyID )
		{
			continue;
		}

		gi.physSystem->GetBodyTransform( ent->bodyID, ent->s.origin, ent->s.angles );

		gi.linkentity( ent );
	}
}

void Phys_SetupPhysicsForEntity( edict_t *ent, bodyID_t bodyID )
{
	ent->movetype = MOVETYPE_PHYSICS;
	ent->bodyID = bodyID;

	gi.physSystem->SetLinearAndAngularVelocity( ent->bodyID, ent->velocity, ent->avelocity );
}

/*
===================================================================================================

	Test Entity

===================================================================================================
*/

static void Think_PhysicsTest( edict_t *self )
{
	//ent->s.frame = ( ent->s.frame + 1 ) % 7;
	self->nextthink = level.time + FRAMETIME;
}

void Spawn_PhysicsTest( edict_t *self )
{
	self->solid = SOLID_NOT;
	VectorSet( self->mins, -16, -16, 0 );
	VectorSet( self->maxs, 16, 16, 72 );
	self->s.modelindex = gi.modelindex( g_viewthing->GetString() );

	bodyID_t bodyID = gi.physSystem->CreateBodySphere( self->s.origin, self->s.angles, 32.0f );

	Phys_SetupPhysicsForEntity( self, bodyID );

	gi.linkentity( self );

	self->think = Think_PhysicsTest;
	self->nextthink = level.time + FRAMETIME;
}
