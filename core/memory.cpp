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

#if defined Q_MEM_USE_MIMALLOC

#define malloc_internal			mi_malloc
#define realloc_internal		mi_realloc
#define free_internal			mi_free

#define msize_internal			mi_usable_size
#define expand_internal			mi_expand

#else

#define malloc_internal			malloc
#define realloc_internal		realloc
#define free_internal			free

#ifdef _WIN32

#define msize_internal			_msize
#define expand_internal			_expand

#else

#error Implement this!

#define msize_internal
#define expand_internal

#endif

#endif

/*
=================================================
	Basic allocation
=================================================
*/

#ifndef Q_MEM_DEBUG

RESTRICTFN void *Mem_Alloc( size_t size )
{
	return malloc_internal( size );
}

RESTRICTFN void *Mem_ReAlloc( void *block, size_t size )
{
	return realloc_internal( block, size );
}

RESTRICTFN void *Mem_ClearedAlloc( size_t size )
{
	void *mem = malloc_internal( size );
	memset( mem, 0, size );
	return mem;
}

RESTRICTFN char *Mem_CopyString( const char *in )
{
	size_t size = strlen( in ) + 1;
	char *out = (char *)malloc_internal( size );
	memcpy( out, in, size );
	return out;
}

size_t Mem_Size( void *block )
{
	return msize_internal( block );
}

void Mem_Free( void *block )
{
	free_internal( block );
}

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
