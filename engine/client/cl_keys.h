
#pragma once

extern char		*keybindings[256];
extern	int		key_repeats[256];

extern	int	anykeydown;
extern char chat_buffer[];
extern	int chat_bufferlen;
extern	qboolean	chat_team;

void Key_Event (int key, qboolean down, unsigned time);
void Key_Init (void);
void Key_WriteBindings (FILE *f);
void Key_SetBinding (int keynum, const char *binding);
void Key_ClearStates (void);
int Key_GetKey (void);
