//-------------------------------------------------------------------------------------------------
// Basic defines extracted from q_shared.h
// Some of these are a best fit for regular q_shared.h, like the MAX_STRING/TOKEN defines
//-------------------------------------------------------------------------------------------------

#pragma once

#if (defined _M_IX86 || defined __i386__) && !defined C_ONLY
#define id386	1
#else
#define id386	0
#endif

#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	80		// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		128		// max length of an individual token

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
