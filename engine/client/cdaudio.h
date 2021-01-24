
#pragma once

bool	CDAudio_Init();
void	CDAudio_Shutdown();
void	CDAudio_Play( int track, bool looping );
void	CDAudio_Stop();
void	CDAudio_Update();
void	CDAudio_Activate( bool active );
