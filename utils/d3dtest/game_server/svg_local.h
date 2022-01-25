
#pragma once

// TODO: q_shared equivalent
#include "../engine.h"

#include <vector>

/*
=======================================
	New Entities

	Entities are entirely allocated by
	the server game code
=======================================
*/

//
// This is how the engine server talks to entities in the game DLL.
// The game's CBaseEntity implements this.
//
abstract_class IServerEntity
{
private:

public:
	/*edict_t m_edict;*/

public:
	IServerEntity() { /*memset( &m_edict, 0, sizeof( m_edict ) );*/ }
	virtual ~IServerEntity() = 0; // TODO: C++23 might have pure virtual destructors

	// These basic accessors return nullptr if the entity has no entity state or player state.
	// If it doesn't have one, then it doesn't need to be sent to clients!
	//virtual entity_state_t *GetEntityState() = 0;
	//virtual player_state_t *GetPlayerState() = 0;
};

#include	"entities.h"

#include	"entities/CBaseEntity.h"			// The root of all evil
#include		"entities/CBaseNetworkable.h"
#include			"entities/CBaseCharacter.h"
#include				"entities/CBasePlayer.h"
