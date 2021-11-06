
#pragma once

extern StaticCvar wnd_width;
extern StaticCvar wnd_height;

void CreateMainWindow( int width, int height );
void DestroyMainWindow();
void ShowMainWindow();
void *GetMainWindow();
