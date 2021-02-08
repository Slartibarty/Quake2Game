// scriplib.h

#pragma once

#include "cmdlib.h"

#define	MAXTOKEN	1024

extern	char	token[MAXTOKEN];
extern	char	*scriptbuffer,*script_p,*scriptend_p;
extern	int		grabbed;
extern	int		scriptline;
extern	qboolean	endofscript;


void LoadScriptFile (char *filename);
void ParseFromMemory (char *buffer, int size);

qboolean GetToken (qboolean crossline);
void UnGetToken (void);
qboolean TokenAvailable (void);


