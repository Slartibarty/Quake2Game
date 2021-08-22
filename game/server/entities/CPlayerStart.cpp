
#include "g_local.h"

class CPlayerStart : public CBaseEntity
{
private:
	DECLARE_CLASS( CPlayerStart, CBaseEntity )

public:
	CPlayerStart() = default;
	~CPlayerStart() override = default;

	void Spawn() override
	{
	}

	void Think() override
	{
	}
};

LINK_ENTITY_TO_CLASS( info_player_start, CPlayerStart )
