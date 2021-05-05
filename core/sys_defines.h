/*
===================================================================================================

	Basic defines

===================================================================================================
*/

#pragma once

#define MAX_PRINT_MSG		1024	// max length of a printf string
#define	MAX_TOKEN_CHARS		128		// max length of an individual token

// TODO: Bump me up! 64 is not enough
#define	MAX_QPATH			64		// max length of a relative game path, IE: ("./base/models/alien.smf")

#ifdef countof
#error countf was previously defined by another library
#endif

#define countof( a )		( sizeof( a ) / sizeof( *a ) )

#define STRINGIFY_( a )		#a
#define STRINGIFY( a )		STRINGIFY_(a)

// max lengths of absolute paths
// win32 has a limit of 256, plus 3 for "C:\" and 1 for the null terminator
// unix typically has a limit of 4096, but for our purposes we're limiting to 1024
// notes:
// all our paths on Linux systems are probably going to be in /home/username/.steam/steam/steamapps/common/moon,
// so we don't need to worry much about path lengths
// before win10, you had to append \?\\ to a path and use only backwards slashes in order to bypass the length limit
// with win10 1607 or so, they removed the path limit limitation, but enablement of this must be set in the registry or GPE,
// we can't set global registry settings as a game engine so that's out of the question, for the record, you must also
// opt into support per-program with a manifest setting

#define WIN32_MAX_PATH		260
#define UNIX_MAX_PATH		1024

#ifdef _WIN32

#define MAX_OSPATH			260		// max length of a filesystem pathname

#define DLLEXPORT			__declspec(dllexport)
#define FORCEINLINE			__forceinline

#else

#define MAX_OSPATH			1024

#define DLLEXPORT			__attribute__((visibility("default")))
#define FORCEINLINE			inline

#endif
