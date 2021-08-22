
#pragma once

//
// This is the base for any entity that must be networked
//
class CBaseNetworkable : public CBaseEntity
{
private:
	DECLARE_CLASS( CBaseNetworkable, CBaseEntity )

protected:
	//entity_state_t m_state;

	moveType_t m_moveType = MOVETYPE_NONE;

public:
	// Constructor
	CBaseNetworkable() = default;
	// Destructor
	~CBaseNetworkable() override = default;

	virtual void Pain() {}
	virtual void Die() {}

	bool IsNetworkable() override { return true; }

	// Server communication
	//entity_state_t *GetEntityState() override { return &m_state; };

};
