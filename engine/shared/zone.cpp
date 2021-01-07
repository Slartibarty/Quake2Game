//=================================================================================================
// Zone memory allocation
//
// Taggged allocations are used by the game code
//=================================================================================================

#include "common.h"
#include "zone.h"

#ifndef Q_RETAIL
#define Q_MEM_DEBUG
#endif

//=================================================================================================
// Basic allocation
//=================================================================================================

#ifdef Q_MEM_DEBUG
static size_t	z_basicbytes;
#endif

//-------------------------------------------------------------------------------------------------
// Allocate memory with no frills
//-------------------------------------------------------------------------------------------------
void *Z_Malloc( size_t size )
{
#ifdef Q_MEM_DEBUG
	z_basicbytes += size;
#endif
	return malloc( size );
}

//-------------------------------------------------------------------------------------------------
// Allocate memory with no frills and set to zero
//-------------------------------------------------------------------------------------------------
void *Z_Calloc( size_t size )
{
#ifdef Q_MEM_DEBUG
	z_basicbytes += size;
#endif
	return calloc( size, 1 );
}

//-------------------------------------------------------------------------------------------------
// Reallocate an existing block
//-------------------------------------------------------------------------------------------------
void *Z_Realloc( void *block, size_t size )
{
#ifdef Q_MEM_DEBUG
	z_basicbytes -= _msize( block );
	z_basicbytes += size;
#endif
	return realloc( block, size );
}

//-------------------------------------------------------------------------------------------------
// Duplicate a string
//-------------------------------------------------------------------------------------------------
char *Z_CopyString( const char *in )
{
	char *out = (char *)Z_Malloc( strlen( in ) + 1 );
	strcpy( out, in );
	return out;
}

//-------------------------------------------------------------------------------------------------
// Free some no-frills memory
//-------------------------------------------------------------------------------------------------
void Z_Free( void *block )
{
#ifdef Q_MEM_DEBUG
	z_basicbytes -= _msize( block );
#endif
	free( block );
}

//=================================================================================================
// Tagged allocation
//=================================================================================================

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

//-------------------------------------------------------------------------------------------------
// Allocate some tracked memory
//-------------------------------------------------------------------------------------------------
void *Z_TagMalloc( size_t size, uint16 tag )
{
	zhead_t *z;

	size += sizeof( zhead_t );
	z = (zhead_t *)malloc( size );
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

//-------------------------------------------------------------------------------------------------
// Free some tagged memory, should this be allowed???
//-------------------------------------------------------------------------------------------------
void Z_TagFree( void *ptr )
{
	zhead_t *z;

	z = (zhead_t *)ptr - 1;

	assert( z->magic == Z_MAGIC );

	z->prev->next = z->next;
	z->next->prev = z->prev;

	--z_tagcount;
	z_tagbytes -= z->size;
	free( z );
}

//-------------------------------------------------------------------------------------------------
// Free all allocations associated with the given tag
//-------------------------------------------------------------------------------------------------
void Z_TagFreeGroup( uint16 tag )
{
	zhead_t *z, *next;

	for ( z = z_tagchain.next; z != &z_tagchain; z = next )
	{
		next = z->next;
		if ( z->tag == tag ) {
			Z_TagFree( (void *)( z + 1 ) );
		}
	}
}

//=================================================================================================
// Status (for tracked memory)
//=================================================================================================

// Concommand to return misc info
void Z_Stats_f()
{
#ifdef Q_MEM_DEBUG
	Com_Printf(
		"Normal allocations: %zu bytes\n"
		"Tag allocations: %zu bytes in %zu blocks\n"
		"Total: %zu bytes\n",
		z_basicbytes,
		z_tagbytes, z_tagcount,
		z_basicbytes + z_tagbytes );
#else
	Com_Printf(
		"Tag allocations: %zu bytes in %zu blocks\n",
		z_tagbytes, z_tagcount );
#endif
}

//-------------------------------------------------------------------------------------------------
// Inits the chain for tagged allocations
//-------------------------------------------------------------------------------------------------
void Z_Init()
{
	z_tagchain.next = z_tagchain.prev = &z_tagchain;
}

//-------------------------------------------------------------------------------------------------
// No purpose yet, eventually will report memory still in use
//-------------------------------------------------------------------------------------------------
void Z_Shutdown()
{
	// Make sure by the time we get here, we don't have any memory still in use...
#ifdef Q_MEM_DEBUG
	// TODO: Figure out why this is always at 16kb or so
	//assert( z_basicbytes == 0 );
#endif
	assert( z_tagbytes == 0 );
	assert( z_tagcount == 0 );
}
