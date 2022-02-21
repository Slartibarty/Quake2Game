
#include "g_local.h"

#include <vector>

/*
===================================================================================================

	Physics

===================================================================================================
*/

static std::vector<shapeHandle_t> s_cachedShapes;

void Phys_CacheShape( shapeHandle_t handle )
{
#ifdef Q_DEBUG
	for ( const shapeHandle_t cachedHandle : s_cachedShapes )
	{
		if ( cachedHandle == handle )
		{
			gi.error( "Tried to re-add an existing handle to s_cachedShapes!" );
		}
	}
#endif

	s_cachedShapes.push_back( handle );
}

void Phys_DeleteCachedShapes()
{
	for ( const shapeHandle_t cachedHandle : s_cachedShapes )
	{
		gi.physSystem->DestroyShape( cachedHandle );
	}
}

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

		gi.physSystem->GetBodyPositionAndRotation( ent->bodyID, ent->s.origin, ent->s.angles );

		gi.linkentity( ent );
	}
}

void Phys_SetupPhysicsForEntity( edict_t *ent, bodyCreationSettings_t &settings, shapeHandle_t shapeHandle )
{
	ent->movetype = MOVETYPE_PHYSICS;
	ent->bodyID = gi.physSystem->CreateAndAddBody( settings, shapeHandle );

	gi.physSystem->SetLinearAndAngularVelocity( ent->bodyID, ent->velocity, ent->avelocity );
}

/*
===================================================================================================

	Test Entity

===================================================================================================
*/

static shapeHandle_t s_physicsTestShape;

static void Think_PhysicsTest( edict_t *self )
{
	//ent->s.frame = ( ent->s.frame + 1 ) % 7;
	self->nextthink = level.time + FRAMETIME;
}

static void Pain_PhysicsTest( edict_t *self, edict_t *other, float kick, int damage )
{

}

static void Die_PhysicsTest( edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point )
{

}

void Spawn_PhysicsTest( edict_t *self )
{
	self->solid = SOLID_BBOX;
	VectorSet( self->mins, -16, -16, -16 );
	VectorSet( self->maxs, 16, 16, 16 );
	self->s.modelindex = gi.modelindex( "models/devtest/metalbox.iqm" );

	self->takedamage = true;
	self->pain = Pain_PhysicsTest;
	self->die = Die_PhysicsTest;

	bodyCreationSettings_t bodyCreationSettings;
	VectorCopy( self->s.origin, bodyCreationSettings.position );
	VectorCopy( self->s.angles, bodyCreationSettings.rotation );
	bodyCreationSettings.friction = 0.77f;

	if ( !s_physicsTestShape )
	{
		vec3_t halfExtent{ 20.0f, 20.0f, 20.0f };
		s_physicsTestShape = gi.physSystem->CreateBoxShape( halfExtent );
		Phys_CacheShape( s_physicsTestShape );
	}

	Phys_SetupPhysicsForEntity( self, bodyCreationSettings, s_physicsTestShape );

	gi.linkentity( self );

	self->think = Think_PhysicsTest;
	self->nextthink = level.time + FRAMETIME;
}
