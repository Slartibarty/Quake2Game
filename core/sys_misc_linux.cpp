/*
===================================================================================================

	Platform specific garbage functions

===================================================================================================
*/

#include "core.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

/*
=======================================
	The hunk allocator, which is not
	long for this world
=======================================
*/

static void *	memBase;
static size_t	hunkMaxSize;
static size_t	curSize;

void *Hunk_Begin( size_t maxSize )
{
	// reserve a huge chunk of memory, but don't commit any yet
	hunkMaxSize = maxSize + sizeof(size_t);
	curSize = 0;
	memBase = mmap(0, hunkMaxSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (memBase == NULL || memBase == (void *)-1)
		Com_FatalErrorf("unable to virtual allocate %zu bytes", maxSize);

	*((size_t *)memBase) = curSize;

	return (byte *)memBase + sizeof(size_t);
}

void *Hunk_Alloc( size_t size )
{
	void *buf;

	// round to cacheline
	size = (size+31)&~31;
	if (curSize + size > hunkMaxSize)
		Com_FatalError("Hunk_Alloc overflow");
	buf = (byte *)memBase + sizeof(size_t) + curSize;
	curSize += size;
	return buf;
}

size_t Hunk_End()
{
	void *n;

	n = mremap(memBase, hunkMaxSize, curSize + sizeof(size_t), 0);
	if (n != memBase)
		Com_FatalErrorf("Hunk_End:  Could not remap virtual block (%d)", errno);
	*((size_t *)memBase) = curSize + sizeof(size_t);
	
	return curSize;
}

void Hunk_Free( void *base )
{
	void *m;

	if (base)
	{
		m = ((byte *)base) - sizeof(size_t);
		if (munmap(m, *((int *)m)))
			Com_FatalErrorf("Hunk_Free: munmap failed (%d)", errno);
	}
}

/*
=======================================
	Miscellaneous
=======================================
*/

void Sys_CopyFile( const char *src, const char *dst )
{
	int srcfile = open(src, O_RDONLY, 0);
	int dstfile = open(dst, O_WRONLY | O_CREAT, 0644);

	struct stat stat_source;
    fstat(srcfile, &stat_source);
	
	sendfile(dstfile, srcfile, nullptr, stat_source.st_size);

	close(dstfile);
	close(srcfile);
}

void Sys_CreateDirectory( const char *path )
{
	mkdir(path, 0777);
}

/*
=======================================
	Timing
=======================================
*/

static int64	startTime;
static double	toSeconds;
static double	toMilliseconds;
static double	toMicroseconds;

int curtime;

void Time_Init()
{
	startTime = SDL_GetPerformanceFrequency();
	toSeconds = 1e0 / startTime;
	toMilliseconds = 1e3 / startTime;
	toMicroseconds = 1e6 / startTime;

	startTime = SDL_GetPerformanceCounter();
}

double Time_FloatSeconds()
{
	int64 currentTime;

	currentTime = SDL_GetPerformanceCounter();

	return ( currentTime - startTime ) * toSeconds;
}

double Time_FloatMilliseconds()
{
	int64 currentTime;

	currentTime = SDL_GetPerformanceCounter();

	return ( currentTime - startTime ) * toMilliseconds;
}

double Time_FloatMicroseconds()
{
	int64 currentTime;

	currentTime = SDL_GetPerformanceCounter();

	return ( currentTime - startTime ) * toMicroseconds;
}

// Practically useless
/*int64 Time_Seconds()
{
	int64 currentTime;

	QueryPerformanceCounter( (LARGE_INTEGER *)&currentTime );

	return llround( ( currentTime - startTime ) * toSeconds );
}*/

int64 Time_Milliseconds()
{
	int64 currentTime;

	currentTime = SDL_GetPerformanceCounter();

	return llround( ( currentTime - startTime ) * toMilliseconds );
}

int64 Time_Microseconds()
{
	int64 currentTime;

	currentTime = SDL_GetPerformanceCounter();

	return llround( ( currentTime - startTime ) * toMilliseconds );
}

// LEGACY
int Sys_Milliseconds()
{
	int64 currentTime;

	currentTime = SDL_GetPerformanceCounter();

	// SlartTodo: Replace int with int64
	curtime = (int)( ( currentTime - startTime ) * toMilliseconds );

	return curtime;
}

/*
=======================================
	Directory searching
=======================================
*/

int glob_match(const char *pattern, const char *text);

/* Like glob_match, but match PATTERN against any final segment of TEXT.  */
static int glob_match_after_star(const char *pattern, const char *text)
{
	const char *p = pattern, *t = text;
	char c, c1;

	while ((c = *p++) == '?' || c == '*')
		if (c == '?' && *t++ == '\0')
			return 0;

	if (c == '\0')
		return 1;

	if (c == '\\')
		c1 = *p;
	else
		c1 = c;

	while (1) {
		if ((c == '[' || *t == c1) && glob_match(p - 1, t))
			return 1;
		if (*t++ == '\0')
			return 0;
	}
}

/* Return nonzero if PATTERN has any special globbing chars in it.  */
static int glob_pattern_p(const char *pattern)
{
	const char *p = pattern;
	char c;
	int open = 0;

	while ((c = *p++) != '\0')
		switch (c) {
		case '?':
		case '*':
			return 1;

		case '[':		/* Only accept an open brace if there is a close */
			open++;		/* brace to match it.  Bracket expressions must be */
			continue;	/* complete, according to Posix.2 */
		case ']':
			if (open)
				return 1;
			continue;

		case '\\':
			if (*p++ == '\0')
				return 0;
		}

	return 0;
}

/* Match the pattern PATTERN against the string TEXT;
   return 1 if it matches, 0 otherwise.

   A match means the entire string TEXT is used up in matching.

   In the pattern string, `*' matches any sequence of characters,
   `?' matches any character, [SET] matches any character in the specified set,
   [!SET] matches any character not in the specified set.

   A set is composed of characters or ranges; a range looks like
   character hyphen character (as in 0-9 or A-Z).
   [0-9a-zA-Z_] is the set of characters allowed in C identifiers.
   Any other character in the pattern must be matched exactly.

   To suppress the special syntactic significance of any of `[]*?!-\',
   and match the character exactly, precede it with a `\'.
*/

int glob_match(const char *pattern, const char *text)
{
	const char *p = pattern, *t = text;
	char c;

	while ((c = *p++) != '\0')
		switch (c) {
		case '?':
			if (*t == '\0')
				return 0;
			else
				++t;
			break;

		case '\\':
			if (*p++ != *t++)
				return 0;
			break;

		case '*':
			return glob_match_after_star(p, t);

		case '[':
			{
				char c1 = *t++;
				int invert;

				if (!c1)
					return (0);

				invert = ((*p == '!') || (*p == '^'));
				if (invert)
					p++;

				c = *p++;
				while (1) {
					char cstart = c, cend = c;

					if (c == '\\') {
						cstart = *p++;
						cend = cstart;
					}
					if (c == '\0')
						return 0;

					c = *p++;
					if (c == '-' && *p != ']') {
						cend = *p++;
						if (cend == '\\')
							cend = *p++;
						if (cend == '\0')
							return 0;
						c = *p++;
					}
					if (c1 >= cstart && c1 <= cend)
						goto match;
					if (c == ']')
						break;
				}
				if (!invert)
					return 0;
				break;

			  match:
				/* Skip the rest of the [...] construct that already matched.  */
				while (c != ']') {
					if (c == '\0')
						return 0;
					c = *p++;
					if (c == '\0')
						return 0;
					else if (c == '\\')
						++p;
				}
				if (invert)
					return 0;
				break;
			}

		default:
			if (c != *t++)
				return 0;
		}

	return *t == '\0';
}

static char		findbase[MAX_OSPATH];
static char		findpath[MAX_OSPATH];
static char		findpattern[MAX_OSPATH];
static DIR		*fdir;

static bool CompareAttributes( const char *path, const char *name, unsigned musthave, unsigned canthave )
{
	struct stat st;
	char fn[MAX_OSPATH];

// . and .. never match
	if (Q_strcmp(name, ".") == 0 || Q_strcmp(name, "..") == 0)
		return false;

	return true;

	if (stat(fn, &st) == -1)
		return false; // shouldn't happen

	if ( ( st.st_mode & S_IFDIR ) && ( canthave & SFF_SUBDIR ) )
		return false;

	if ( ( musthave & SFF_SUBDIR ) && !( st.st_mode & S_IFDIR ) )
		return false;

	return true;
}

char *Sys_FindFirst (const char *path, unsigned musthave, unsigned canthave )
{
	struct dirent *d;
	char *p;

	if (fdir)
		Com_FatalError ("Sys_BeginFind without close");

//	COM_FilePath (path, findbase);
	Q_strcpy_s(findbase, path);

	if ((p = strrchr(findbase, '/')) != NULL) {
		*p = 0;
		Q_strcpy_s(findpattern, p + 1);
	} else
		Q_strcpy_s(findpattern, "*");

	if (Q_strcmp(findpattern, "*.*") == 0)
		Q_strcpy_s(findpattern, "*");
	
	if ((fdir = opendir(findbase)) == NULL)
		return NULL;
	while ((d = readdir(fdir)) != NULL) {
		if (!*findpattern || glob_match(findpattern, d->d_name)) {
//			if (*findpattern)
//				printf("%s matched %s\n", findpattern, d->d_name);
			if (CompareAttributes(findbase, d->d_name, musthave, canthave)) {
				Q_sprintf_s (findpath, "%s/%s", findbase, d->d_name);
				return findpath;
			}
		}
	}
	return NULL;
}

char *Sys_FindNext (unsigned musthave, unsigned canthave)
{
	struct dirent *d;

	if (fdir == NULL)
		return NULL;
	while ((d = readdir(fdir)) != NULL) {
		if (!*findpattern || glob_match(findpattern, d->d_name)) {
//			if (*findpattern)
//				printf("%s matched %s\n", findpattern, d->d_name);
			if (CompareAttributes(findbase, d->d_name, musthave, canthave)) {
				Q_sprintf_s (findpath, "%s/%s", findbase, d->d_name);
				return findpath;
			}
		}
	}
	return NULL;
}

void Sys_FindClose (void)
{
	if (fdir != NULL)
		closedir(fdir);
	fdir = NULL;
}
