/*
===================================================================================================

	Memory management

	Tagged allocation is used by the game code

===================================================================================================
*/

#include "core.h"

#if defined Q_MEM_USE_MIMALLOC
#include "../../thirdparty/mimalloc/include/mimalloc.h"
#endif

#include "memory.h"

//#define NO_OVERRIDE_NEWDELETE

#if defined Q_MEM_USE_MIMALLOC

#define malloc_internal			mi_malloc
#define realloc_internal		mi_realloc
#define free_internal			mi_free

#else

#define malloc_internal			malloc
#define realloc_internal		realloc
#define free_internal			free

#endif

/*
=================================================
	Basic allocation
=================================================
*/

#ifndef Q_MEM_DEBUG

void *Mem_Alloc( size_t size )
{
	return malloc_internal( size );
}

void *Mem_ReAlloc( void *block, size_t size )
{
	return realloc_internal( block, size );
}

void *Mem_ClearedAlloc( size_t size )
{
	void *mem = Mem_Alloc( size );
	memset( mem, 0, size );
	return mem;
}

char *Mem_CopyString( const char *in )
{
	char *out = (char *)Mem_Alloc( strlen( in ) + 1 );
	strcpy( out, in );
	return out;
}

void Mem_Free( void *block )
{
	free_internal( block );
}

/*
=================================================
	C++ new and delete

	This overrides both versions of new and delete
	need to check if MSVC has any special cases
=================================================
*/

#ifndef NO_OVERRIDE_NEWDELETE

// double *pMem = new double;
void *operator new( size_t size )
{
	return Mem_Alloc( size );
}

// delete pMem;
void operator delete( void *pMem )
{
	Mem_Free( pMem );
}

// char *pMem = new char[256];
void *operator new[]( size_t size )
{
	return Mem_Alloc( size );
}

// delete[] pMem;
void operator delete[]( void *pMem )
{
	Mem_Free( pMem );
}

#endif

#endif

/*
=================================================
	Tagged allocation
=================================================
*/

static constexpr uint16 Z_MAGIC = 0x1d1d;

// 24-byte header
struct zhead_t
{
	zhead_t		*prev, *next;
	uint16		magic;
	uint16		tag;			// For group free
	uint32		size;			// Size of this allocation in bytes, could use _msize
};

static zhead_t	z_tagchain;
static size_t	z_tagcount, z_tagbytes;

// Allocate some memory with a tag at the front
void *Mem_TagAlloc( size_t size, uint16 tag )
{
	zhead_t *z;

	size += sizeof( zhead_t );
	z = (zhead_t *)Mem_Alloc( size );
	assert( z );

	++z_tagcount;
	z_tagbytes += size;

	z->magic = Z_MAGIC;
	z->tag = tag;
	z->size = static_cast<uint32>( size );

	z->next = z_tagchain.next;
	z->prev = &z_tagchain;
	z_tagchain.next->prev = z;
	z_tagchain.next = z;

	return (void *)( z + 1 );
}

// Free a single tag allocation
void Mem_TagFree( void *block )
{
	zhead_t *z;

	z = (zhead_t *)block - 1;

	assert( z->magic == Z_MAGIC );

	z->prev->next = z->next;
	z->next->prev = z->prev;

	--z_tagcount;
	z_tagbytes -= z->size;
	Mem_Free( z );
}

// Free all allocations associated with the given tag
void Mem_TagFreeGroup( uint16 tag )
{
	zhead_t *z, *next;

	for ( z = z_tagchain.next; z != &z_tagchain; z = next )
	{
		next = z->next;
		if ( z->tag == tag ) {
			Mem_TagFree( (void *)( z + 1 ) );
		}
	}
}

/*
=================================================
	Status
=================================================
*/

void Mem_Init()
{
	z_tagchain.next = z_tagchain.prev = &z_tagchain;
}

void Mem_Shutdown()
{
	
}

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

	/*
	=================================================
		CRT internals
	=================================================
	*/

	// This is causing trouble with release builds
#if 0
	size_t _msize( void *pMem )
	{
		return mi_usable_size( pMem );
	}
#endif

}

#endif
