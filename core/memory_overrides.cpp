/*
===================================================================================================

	Overrides standard (and non-standard) memory allocation functions

===================================================================================================
*/

#include "../core/memory_impl.h"
#include "../core/memory.h"

#include "../thirdparty/mimalloc/include/mimalloc.h"

#ifdef Q_MEM_USE_MIMALLOC

/*
=================================================
	C++ new and delete

	This overrides both versions of new and delete
	need to check if MSVC has any special cases
=================================================
*/

#ifndef NO_OVERRIDE_NEWDELETE

#if 1 // Q_MEM_USE_MIMALLOC

// double *pMem = new double;
void *operator new( size_t size )
{
	return Mem_Alloc( size );
}

// char *pMem = new char[256];
void *operator new[]( size_t size )
{
	return Mem_Alloc( size );
}

// delete pMem;
void operator delete( void *pMem )
{
	Mem_Free( pMem );
}

// delete[] pMem;
void operator delete[]( void *pMem )
{
	Mem_Free( pMem );
}

#else

#include "../../thirdparty/mimalloc/include/mimalloc-new-delete.h"

#endif

#endif

/*
===============================================================================
	The gross stuff

	MSVCRT specific allocator overrides, so
	third-party libraries that call plain old
	"malloc" go through this allocator.

	If using this without a custom allocator, it'll result in an infinite loop.
	(malloc calls Mem_Alloc calls malloc calls Mem_Alloc etc)

	TODO: This doesn't yet account for internal functions, a screwy library
	directly accessing MSVCRT functions internally would suck!
===============================================================================
*/

#if defined _WIN32 && defined Q_MEM_USE_MIMALLOC

extern "C"
{
	/*
	=================================================
		Standard functions
	=================================================
	*/

	RESTRICTFN void *malloc( size_t size )
	{
		return Mem_Alloc( size );
	}

	RESTRICTFN void *realloc( void *pMem, size_t size )
	{
		return Mem_ReAlloc( pMem, size );
	}

	RESTRICTFN void *calloc( size_t count, size_t size )
	{
		return Mem_ClearedAlloc( count * size );
	}

	void free( void *pMem )
	{
		Mem_Free( pMem );
	}

#ifndef _WIN32 // future proofing

	RESTRICTFN char *strdup( const char *pString )
	{
		return mi_strdup( pString );
	}

	RESTRICTFN char *strndup( const char *pString, size_t count )
	{
		return mi_strndup( pString, count );
	}

#endif

	/*
	=================================================
		MSVCRT specific

		There are so many bullshit functions I can't
		be ARSED to implement them now, I bet the
		external libraries don't even USE them, GOD
	=================================================
	*/

	size_t _msize( void *pMem )
	{
		return Mem_Size( pMem );
	}

#ifdef Q_DEBUG

	RESTRICTFN char *_strdup_dbg( const char *pString, int block_use, const char *file_name, int line_number )
	{
		return Mem_CopyString( pString );
	}

#endif

	RESTRICTFN char *_strdup( const char *pString )
	{
		return Mem_CopyString( pString );
	}

	// CRT hack
	RESTRICTFN void *_recalloc_base( void *pBlock, size_t count, size_t size )
	{
		void *mem = Mem_ReAlloc( pBlock, count * size );
		memset( mem, 0, count * size );
		return mem;
	}

	RESTRICTFN void *_recalloc( void *pBlock, size_t count, size_t size )
	{
		void *mem = Mem_ReAlloc( pBlock, count * size );
		memset( mem, 0, count * size );
		return mem;
	}

	// CRT hack
	RESTRICTFN void *_expand_base( void *pMem, size_t size )
	{
		return mi_expand( pMem, size );
	}

	RESTRICTFN void *_expand( void *pMem, size_t size )
	{
		return mi_expand( pMem, size );
	}

}

#endif

#endif
