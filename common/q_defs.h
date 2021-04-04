//-------------------------------------------------------------------------------------------------
// Basic defines extracted from q_shared.h
//-------------------------------------------------------------------------------------------------

#pragma once

#define MAX_PRINT_MSG		1024	// max length of a printf string

#define	MAX_QPATH			64		// max length of a quake game pathname

#ifdef countof
#error
#endif

#define countof(a) (sizeof(a) / sizeof(*a))

#define STRINGIFY_(a) #a
#define STRINGIFY(a) STRINGIFY_(a)

#ifdef _WIN32

#define MAX_OSPATH			260		// max length of a filesystem pathname

#define DLLEXPORT __declspec(dllexport)
#define FORCEINLINE __forceinline

#else

#define MAX_OSPATH			1024	// max length of a filesystem pathname

#define DLLEXPORT
#define FORCEINLINE inline

#endif
