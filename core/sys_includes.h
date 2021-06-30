/*
===================================================================================================

	System specific includes

	Currently we only support Windows desktop and Linux desktop

===================================================================================================
*/

#pragma once

#ifdef _WIN32

// Windows!

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#else

// Linux!

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#endif
