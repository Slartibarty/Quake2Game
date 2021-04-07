//=================================================================================================
// The console
//=================================================================================================

#pragma once

void DrawString( int x, int y, const char *s );
void DrawAltString( int x, int y, const char *s );	// Green

void Con_CheckResize();
void Con_Init();
void Con_DrawConsole( float frac );
void Con_Print( const char *txt );
void Con_Clear_f();
void Con_DrawNotify();
void Con_ClearNotify();
void Con_ToggleConsole_f();

bool Con_IsInitialized();

// Q3 added
void Con_PageUp();
void Con_PageDown();
void Con_Top();
void Con_Bottom();
