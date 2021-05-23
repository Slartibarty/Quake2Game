
#pragma once

extern char		*keybindings[256];
extern int		key_repeats[256];

extern int		anykeydown;
extern char		chat_buffer[];
extern int		chat_bufferlen;
extern bool		chat_team;

// called by engine, not client, these declarations are never referenced
void Key_Init();
void Key_Shutdown();

void Key_Event (int key, bool down, unsigned time);
void Key_WriteBindings (FILE *f);
void Key_SetBinding (int keynum, const char *binding);
void Key_ClearStates (void);
