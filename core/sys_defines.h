/*
===================================================================================================

	Basic defines

===================================================================================================
*/

#pragma once

#ifdef countof
#error countf was previously defined by another library
#endif

#define MAX_PRINT_MSG		1024	// max length of a printf string
#define	MAX_TOKEN_CHARS		128		// max length of an individual token

// TODO: Bump me up! 64 is not enough
// TODO: These are both the same thing, replace QPATH or make assetpath QPATH
#define	MAX_QPATH			64		// max length of a relative mod path, IE: ("models/alien.smf")
#define MAX_ASSETPATH		128		// max length of a mod assetname, IE: ("models/devtest/alyx.smf")

#define STRINGIFY_(a)		#a
#define STRINGIFY(a)		STRINGIFY_(a)

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
#define LONG_MAX_PATH		UNIX_MAX_PATH		// when we need a long pathname and don't care about os crap

#ifdef _WIN32

#define MAX_OSPATH			WIN32_MAX_PATH		// max length of a filesystem pathname

#define DLLEXPORT			__declspec(dllexport)
#define RESTRICTFN			__declspec(restrict)
#define RESTRICT			__restrict
#define FORCEINLINE			__forceinline
#define ASSUME_(x)			__assume(x)

#define abstract_class		class __declspec(novtable)

#else

#define MAX_OSPATH			UNIX_MAX_PATH

#define DLLEXPORT			__attribute__((visibility("default")))
#define RESTRICTFN
#define RESTRICT			__restrict__
#define FORCEINLINE			inline
#define ASSUME_(x)

#define abstract_class		class

#endif

// reminder for me to check my bookmarks for that assume compiler performance stuff

#ifdef Q_DEBUG

#define ASSUME(x)			assert(x)

#else

#define ASSUME(x)			ASSUME_(x)

#endif
