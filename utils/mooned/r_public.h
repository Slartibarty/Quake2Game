
#pragma once

#pragma push_macro("countof")
#undef countof

#define GLM_FORCE_EXPLICIT_CTOR
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#pragma pop_macro("countof")

#include "meshbuilder.h"

/*
=============================
	r_main.cpp
=============================
*/

void R_Init();
void R_Shutdown();

/*
=============================
	r_utils.cpp
=============================
*/

void R_Clear();

/*
=============================
	r_prog.cpp
=============================
*/

struct glProgs_t
{
	GLuint dbgProg;
};

void Shaders_Init( glProgs_t &glProgs );
void Shaders_Shutdown( glProgs_t &glProgs );
