//-------------------------------------------------------------------------------------------------
// Basic defines extracted from q_shared.h
//-------------------------------------------------------------------------------------------------

#pragma once

#include <cstdlib>

#if (defined _M_IX86 || defined __i386__) && !defined C_ONLY
	#define id386	1
#else
	#define id386	0
#endif

#ifndef countof
	#ifdef _WIN32
		#define countof _countof
	#else
		#define countof(a) (sizeof(a) / sizeof(a[0]))
	#endif
#endif

#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	80		// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		128		// max length of an individual token

#define MAX_PRINT_MSG		1024	// max length of a printf string

#define	MAX_QPATH			64		// max length of a quake game pathname
#define	MAX_OSPATH			260		// max length of a filesystem pathname
