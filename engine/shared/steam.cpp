/*
===================================================================================================

	Steam Integration

	Would be better to create split implementations for the client and server
	I.E. SteamGameManager_Client and SteamGameManager_Server

===================================================================================================
*/

#include "engine.h"

#ifdef Q_USE_STEAM

#include "steam/steam_api.h"
#include "steam/isteamuserstats.h"

#include "steam.h"

namespace Steam
{

/*
========================
	SteamGameManager
========================
*/

class SteamGameManager
{
private:
	STEAM_CALLBACK( SteamGameManager, OnGameOverlayActivated, GameOverlayActivated_t );

	STEAM_CALLBACK( SteamGameManager, OnUserStatsReceived, UserStatsReceived_t );
	STEAM_CALLBACK( SteamGameManager, OnUserStatsStored, UserStatsStored_t );

public:
	uint64 m_appID = 0;
	bool m_recievedUserStats = false;
};

static SteamGameManager sgm;

void SteamGameManager::OnGameOverlayActivated( GameOverlayActivated_t *pCallback )
{
	if ( dedicated->GetBool() )
	{
		return;
	}

	Assert( Com_IsMainThread() );

	Com_Printf( "%s the Steam overlay\n", pCallback->m_bActive ? "Opening" : "Closing" );

	extern void CL_Pause_f();
	CL_Pause_f();
}

void SteamGameManager::OnUserStatsReceived( UserStatsReceived_t *pCallback )
{
	if ( pCallback->m_nGameID != m_appID )
	{
		return;
	}

	if ( pCallback->m_eResult == k_EResultOK )
	{
		m_recievedUserStats = true;

		Com_Print( "Received stats and achievements from Steam\n" );
	}
}

void SteamGameManager::OnUserStatsStored( UserStatsStored_t *pCallback )
{
	if ( pCallback->m_nGameID != m_appID )
	{
		return;
	}
}

//=================================================================================================

void Frame()
{
	SteamAPI_RunCallbacks();
}

void Init()
{
	sgm.m_appID = SteamUtils()->GetAppID();

	Assert( SteamUserStats() && SteamUser() );
	Assert( SteamUser()->BLoggedOn() );

	bool result = SteamUserStats()->RequestCurrentStats();
	Assert( result );
}

void Shutdown()
{

}

//=================================================================================================

bool SetAchievement( const char *ach )
{
	return SteamUserStats()->SetAchievement( ach );
}

bool ClearAchievement( const char *ach )
{
	return SteamUserStats()->ClearAchievement( ach );
}

bool StoreStats()
{
	return SteamUserStats()->StoreStats();
}

} // namespace Steam

#endif
