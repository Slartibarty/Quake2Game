
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
	// Players don't use this function
	if ( g_playersOnly->GetBool() ) {
		return;
	}

	gi.physSystem->Simulate( deltaTime );

	for ( int i = game.maxclients + 1; i < globals.num_edicts; ++i )
	{
		edict_t *ent = g_edicts + i;

		if ( !ent->bodyID )
		{
			continue;
		}

		gi.physSystem->GetBodyPositionAndRotation( ent->bodyID, ent->s.origin, ent->s.angles );
		gi.physSystem->GetLinearAndAngularVelocity( ent->bodyID, ent->velocity, ent->avelocity );

		gi.linkentity( ent );
	}
}

void Phys_SetupPhysicsForEntity( edict_t *ent, const bodyCreationSettings_t &settings, shapeHandle_t shapeHandle )
{
	ent->movetype = MOVETYPE_PHYSICS;
	ent->bodyID = gi.physSystem->CreateAndAddBody( settings, shapeHandle, ent );

	gi.physSystem->SetLinearAndAngularVelocity( ent->bodyID, ent->velocity, ent->avelocity );
}

void Phys_Impact( edict_t *ent1, edict_t *ent2, cplane_t *plane, csurface_t *surf )
{
	if ( ent1->touch && ent1->solid != SOLID_NOT )
	{
		ent1->touch( ent1, ent2, plane, surf );
	}

	if ( ent2->touch && ent2->solid != SOLID_NOT )
	{
		ent2->touch( ent2, ent1, plane, surf );
	}
}

/*
===================================================================================================

	Test Entity

===================================================================================================
*/

static shapeHandle_t s_cubeShape;
static shapeHandle_t s_oilDrumShape;

static int HackRandom( int low, int high )
{
	int range;

	range = high - low + 1;
	if ( !( range - 1 ) )
	{
		return low;
	}
	else
	{
		int offset;
		int rnum;

		rnum = rand();

		offset = rnum % range;

		return ( low + offset );
	}
}

static void Think_Any( edict_t *self )
{
	//ent->s.frame = ( ent->s.frame + 1 ) % 7;
	self->nextthink = level.time + FRAMETIME;
}

static void Touch_Cube( edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf )
{
	float speed = fabsf( self->velocity[0] ) + fabsf( self->velocity[1] ) + fabsf( self->velocity[2] );
	float volume = Min( speed * ( 1.0f / ( 320.0f * 320.0f ) ), 1.0f );

	const char *fmtString;

	if ( speed > 100000.0f )
	{
		fmtString = "physics/metal_solid_impact_hard%d.wav";
		Com_Printf( "Hard collision with speed: %4.2f\n", speed );
	}
	else
	{
		fmtString = "physics/metal_solid_impact_soft%d.wav";
		Com_Printf( "Soft collision with speed: %4.2f\n", speed );
	}

	char soundName[MAX_QPATH];
	Q_sprintf_s( soundName, fmtString, HackRandom( 1, 3 ) );

	//Com_Printf( "%4.2f\n", length );

	gi.sound( self, CHAN_AUTO, gi.soundindex( soundName ), volume, ATTN_NORM, 0.0f );
}

static void Pain_Null( edict_t *self, edict_t *other, float kick, int damage )
{

}

static void Die_Null( edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point )
{

}

void Spawn_PhysCube( edict_t *self )
{
	self->solid = SOLID_BBOX;
	VectorSet( self->mins, -16, -16, -16 );
	VectorSet( self->maxs, 16, 16, 16 );
	self->s.modelindex = gi.modelindex( "models/props/metalbox.iqm" );

	self->takedamage = true;
	self->touch = Touch_Cube;
	self->pain = Pain_Null;
	self->die = Die_Null;

	bodyCreationSettings_t bodyCreationSettings;
	VectorCopy( self->s.origin, bodyCreationSettings.position );
	VectorCopy( self->s.angles, bodyCreationSettings.rotation );
	bodyCreationSettings.friction = 0.77f;
	bodyCreationSettings.restitution = 0.13f;

	if ( !s_cubeShape )
	{
		vec3_t halfExtent{ 20.0f, 20.0f, 20.0f };
		s_cubeShape = gi.physSystem->CreateBoxShape( halfExtent );
		Phys_CacheShape( s_cubeShape );
	}

	Phys_SetupPhysicsForEntity( self, bodyCreationSettings, s_cubeShape );

	gi.linkentity( self );

	self->think = Think_Any;
	self->nextthink = level.time + FRAMETIME;
}

void Spawn_PhysBarrel( edict_t *self )
{
	self->solid = SOLID_BBOX;
	VectorSet( self->mins, -16, -16, -16 );
	VectorSet( self->maxs, 16, 16, 16 );
	self->s.modelindex = gi.modelindex( "models/props/oildrum.iqm" );

	self->takedamage = true;
	self->touch = Touch_Cube;
	self->pain = Pain_Null;
	self->die = Die_Null;

	bodyCreationSettings_t bodyCreationSettings;
	VectorCopy( self->s.origin, bodyCreationSettings.position );
	VectorCopy( self->s.angles, bodyCreationSettings.rotation );
	bodyCreationSettings.friction = 0.6f;
	bodyCreationSettings.restitution = 0.2f;

	if ( !s_oilDrumShape )
	{
		s_oilDrumShape = gi.physSystem->CreateCylinderShape( 22.5f, 13.75f );
		Phys_CacheShape( s_oilDrumShape );
	}

	Phys_SetupPhysicsForEntity( self, bodyCreationSettings, s_oilDrumShape );

	gi.linkentity( self );

	self->think = Think_Any;
	self->nextthink = level.time + FRAMETIME;
}
