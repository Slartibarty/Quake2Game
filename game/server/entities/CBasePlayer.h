
#pragma once

//
// This is the player base
//
class CBasePlayer : public CBaseCharacter
{
private:
	DECLARE_CLASS( CBasePlayer, CBaseCharacter )

public:
	//player_state_t m_plr;
	gclient_t m_client;

public:
	// Constructor
	CBasePlayer() = default;
	// Destructor
	~CBasePlayer() override = default;

	// Server communication
	//player_state_t *GetPlayerState() override { return &m_plr; };

};
