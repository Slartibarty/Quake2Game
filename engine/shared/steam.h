/*
===================================================================================================

	Steam Integration

===================================================================================================
*/

#pragma once

namespace Steam
{
#ifdef Q_USE_STEAM

void Frame();
void Init();
void Shutdown();

bool SetAchievement( const char *ID );
bool ClearAchievement( const char *ach );
bool StoreStats();

#else

void Frame() {}
void Init() {}
void Shutdown() {}

bool SetAchievement( const char *ID ) { return false; }
bool ClearAchievement( const char *ach ) { return false; }
bool StoreStats() { return false; }

#endif
}
