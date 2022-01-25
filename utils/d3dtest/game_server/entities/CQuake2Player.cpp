
#include "../svg_local.h"

//
// This is the Quake 2 player
//
class CQuake2Player : public CBasePlayer
{
private:
	DECLARE_CLASS( CQuake2Player, CBasePlayer )

public:
	// Constructor
	CQuake2Player() = default;
	// Destructor
	~CQuake2Player() override = default;

	void Pain() override
	{

	}

	void Die() override
	{

	}

};

LINK_ENTITY_TO_CLASS( player, CQuake2Player )
