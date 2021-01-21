//-------------------------------------------------------------------------------------------------
// Basic defines extracted from q_shared.h
//-------------------------------------------------------------------------------------------------

#pragma once

#define id386	0

#define MAX_PRINT_MSG		1024	// max length of a printf string

#define	MAX_QPATH			64		// max length of a quake game pathname
#define	MAX_OSPATH			260		// max length of a filesystem pathname

#ifndef countof
#define countof(a) (sizeof(a) / sizeof(*a))
#endif

#define STRINGIFY(a) #a
#define XSTRINGIFY(a) STRINGIFY(a)

#ifdef _WIN32	// MSVC / Clang-CL

#define DLLEXPORT __declspec(dllexport)
#define FORCEINLINE __forceinline

#else	// GCC / Clang

#error
#define DLLEXPORT IMPLEMENT_THIS_MACRO
#define FORCEINLINE inline

#endif
