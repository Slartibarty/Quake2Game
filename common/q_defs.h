//-------------------------------------------------------------------------------------------------
// Basic defines extracted from q_shared.h
//-------------------------------------------------------------------------------------------------

#pragma once

#define id386	0

#define MAX_PRINT_MSG		1024	// max length of a printf string

#define	MAX_QPATH			64		// max length of a quake game pathname

#ifndef countof
#define countof(a) (sizeof(a) / sizeof(*a))
#endif

#define STRINGIFY(a) #a
#define XSTRINGIFY(a) STRINGIFY(a)

#ifdef _DEBUG
#define Q_DEBUG
#endif

#ifdef _WIN32	// MSVC / Clang-CL

#define MAX_OSPATH			260		// max length of a filesystem pathname

#define DLLEXPORT __declspec(dllexport)
#define FORCEINLINE __forceinline

#else	// GCC / Clang

#define MAX_OSPATH			1024	// max length of a filesystem pathname

#define DLLEXPORT
#define FORCEINLINE inline

#endif
