
#pragma once

//
// This is an entity that does nothing, it is not linked to any class,
// but another entity may link it under an alias (IE: some entity that needs no state)
//
class CBaseEntity : public IServerEntity
{
private:
	DECLARE_CLASS_NOBASE( CBaseEntity )

	// No locals

public:
	// Constructor
	CBaseEntity() = default;
	// Destructor
	~CBaseEntity() override = default;

	// Spawn, called when the entity is instantiated
	virtual void Spawn() {}
	// Think, called each frame, once for each entity in the scene
	virtual void Think() {}

	// Called when saving information to a file
	virtual void Save() {}
	// Called when loading information from a file
	virtual void Load() {}

	// Feed me keyvalues!
	virtual void KeyValue( const char *pKey, const char *pValue ) {}

	virtual bool IsNetworkable() { return false; }

	// Server communication
	//entity_state_t *GetEntityState() override { return nullptr; };
	//player_state_t *GetPlayerState() override { return nullptr; };

};
