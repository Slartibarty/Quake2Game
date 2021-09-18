/*
===================================================================================================

	Overrides standard (and non-standard) memory allocation functions

===================================================================================================
*/

#include "../core/memory_impl.h"

#ifdef Q_MEM_USE_MIMALLOC

#include "../core/memory.h"

#include "../thirdparty/mimalloc/include/mimalloc.h"

/*
=================================================
	C++ new and delete

	This overrides both versions of new and delete
	need to check if MSVC has any special cases
=================================================
*/

#ifndef NO_OVERRIDE_NEWDELETE

#if 1

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

#ifdef _WIN32

void *operator new( size_t size, int blockUse, const char *pFileName, int line )
{
	return Mem_Alloc( size );
}

void *operator new[]( size_t size, int blockUse, const char *pFileName, int line )
{
	return Mem_Alloc( size );
}

#endif

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

#if defined _WIN32 && defined Q_MEM_USE_MIMALLOC && !defined NO_MEM_OVERRIDE

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

#ifndef Q_DEBUG
	RESTRICTFN void* _malloc_dbg(
		size_t      size,
		int         block_use,
		char const* file_name,
		int         line_number
	)
	{
		return Mem_Alloc( size );
	}
#endif

	RESTRICTFN void *_malloc_base( size_t size )
	{
		return Mem_Alloc( size );
	}

#ifndef Q_DEBUG
	RESTRICTFN void* _realloc_dbg(
		void*       block,
		size_t      size,
		int         block_use,
		char const* file_name,
		int         line_number
	)
	{
		return Mem_ReAlloc( block, size );
	}
#endif

	RESTRICTFN void* _realloc_base( void* block, size_t size )
	{
		return Mem_ReAlloc( block, size );
	}

#ifndef Q_DEBUG
	RESTRICTFN void *_calloc_dbg(
		size_t		count,
		size_t		size,
		int			block_use,
		char const*	file_name,
		int         line_number
	)
	{
		return Mem_ClearedAlloc( count * size );
	}
#endif

	RESTRICTFN void* _calloc_base( size_t count, size_t size )
	{
		return Mem_ClearedAlloc( count * size );
	}

#ifndef Q_DEBUG
	extern "C" __declspec( noinline ) void __cdecl _free_dbg( void* const block, int const block_use )
	{
		Mem_Free( block );
	}
#endif

	void _free_base( void *block )
	{
		Mem_Free( block );
	}

	//=================================

#ifndef Q_DEBUG
	size_t _msize_dbg( void* block, int block_use )
	{
		return Mem_Size( block );
	}
#endif

	size_t _msize_base( void *block ) noexcept
	{
		return Mem_Size( block );
	}

	size_t _msize( void *block )
	{
		return Mem_Size( block );
	}

	RESTRICTFN char *_strdup_dbg( const char *pString, int block_use, const char *file_name, int line_number )
	{
		return Mem_CopyString( pString );
	}

	RESTRICTFN char *_strdup( const char *pString )
	{
		return Mem_CopyString( pString );
	}

#ifndef Q_DEBUG
	void* _recalloc_dbg(
		void*       block,
		size_t      count,
		size_t      element_size,
		int         block_use,
		char const* file_name,
		int         line_number
	)
	{
		void *mem = Mem_ReAlloc( block, count * element_size );
		memset( mem, 0, count * element_size );
		return mem;
	}
#endif

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

#ifndef Q_DEBUG
	void* _expand_dbg(
		void*       block,
		size_t      requested_size,
		int         block_use,
		char const* file_name,
		int         line_number
	)
	{
		return mi_expand( block, requested_size );
	}
#endif

	// CRT hack
	RESTRICTFN void *_expand_base( void *pMem, size_t size )
	{
		return mi_expand( pMem, size );
	}

	RESTRICTFN void *_expand( void *pMem, size_t size )
	{
		return mi_expand( pMem, size );
	}

	/*
	=================================================
		CRT gubbins
	=================================================
	*/

	RESTRICTFN void *_malloc_crt( size_t size )
	{
		return malloc( size );
	}

	RESTRICTFN void *_calloc_crt( size_t count, size_t size )
	{
		return calloc( count, size );
	}

	RESTRICTFN void *_realloc_crt( void *ptr, size_t size )
	{
		return realloc( ptr, size );
	}

	RESTRICTFN void *_recalloc_crt( void *ptr, size_t count, size_t size )
	{
		return _recalloc( ptr, count, size );
	}

	//===============================================

	using HANDLE = void *;

	unsigned int _amblksiz = 16; //BYTES_PER_PARA;

	HANDLE _crtheap = (HANDLE)1;	// PatM Can't be 0 or CRT pukes
	int __active_heap = 1;

	size_t _get_sbh_threshold()
	{
		return 0;
	}

	int __cdecl _set_sbh_threshold( size_t bruh )
	{
		return 0;
	}

	int _heapchk()
	{
		return 1;
	}

	int _heapmin()
	{
		return 1;
	}

	int _heapadd( void *bruh1, size_t bruh2 )
	{
		return 0;
	}

	int _heapset( unsigned int bruh )
	{
		return 0;
	}

	size_t _heapused( size_t *bruh1, size_t *bruh2 )
	{
		return 0;
	}

	int _heapwalk( struct _heapinfo * )
	{
		return 0;
	}

	//===============================================

	int _heap_init()
	{
		return 1;
	}

	void _heap_term()
	{
	}

}

#endif

#endif // #ifndef Q_DEBUG
