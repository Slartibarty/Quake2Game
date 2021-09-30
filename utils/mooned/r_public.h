
#pragma once

#pragma push_macro("countof")
#undef countof

#define GLM_FORCE_EXPLICIT_CTOR
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#pragma pop_macro("countof")

/*
=============================
	r_main.cpp
=============================
*/

struct matrices_t
{
	glm::mat4 view;
	glm::mat4 projection;
};

extern matrices_t g_matrices;

extern QOpenGLFunctions_3_3_Compatibility *qgl;

void R_Init();
void R_Shutdown();

/*
=============================
	r_utils.cpp
=============================
*/

void R_Clear();
void R_DrawWorldAxes();
void R_DrawCrosshair( int windowWidth, int windowHeight );
void R_DrawFrameRect( int windowWidth, int windowHeight );

/*
=============================
	r_prog.cpp
=============================
*/

struct glProgs_t
{
	GLuint dbgProg;
};

extern glProgs_t glProgs;
