
#pragma once

//
// A character
//
class CBaseCharacter : public CBaseNetworkable
{
private:
	DECLARE_CLASS( CBaseCharacter, CBaseNetworkable )

protected:
	//player_state_t m_plr;

public:
	// Constructor
	CBaseCharacter() = default;
	// Destructor
	~CBaseCharacter() override = default;

	// Server communication
	//player_state_t *GetPlayerState() override { return &m_plr; };

};
