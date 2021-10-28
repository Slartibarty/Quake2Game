
#pragma once

struct glProgs_t
{
	GLuint dbgProg;
	GLuint dbgPolyProg;
};

extern glProgs_t glProgs;

void Shaders_Init();
void Shaders_Shutdown();

