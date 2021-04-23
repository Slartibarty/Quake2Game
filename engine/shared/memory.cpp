/*
===================================================================================================

	Memory management

	Tagged allocation is used by the game code

===================================================================================================
*/

#include "engine.h"

#include "memory.h"

#include <cstdio>
#include <cstdlib>

#if 0
static constexpr size_t MemAlignment = 16;
static constexpr size_t MemShift = MemAlignment - 1;

static constexpr size_t ALIGN( size_t size ) { return ( size + MemShift ) & ~MemShift; }
#endif

/*
=================================================
	Basic allocation
=================================================
*/

void *Mem_Alloc( size_t size )
{
	return malloc( size );
}

void *Mem_ReAlloc( void *block, size_t size )
{
	return realloc( block, size );
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
	free( block );
}

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
	z->size = size;

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

static void Mem_Stats_f()
{
	Com_Print( "Not implemented\n" );
}

void Mem_Init()
{
	z_tagchain.next = z_tagchain.prev = &z_tagchain;

	Cmd_AddCommand( "mem_stats", Mem_Stats_f );
}

void Mem_Shutdown()
{
	
}
