/*
===================================================================================================

	Memory management

===================================================================================================
*/

#pragma once

#include <cstddef> // size_t

#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif

// Stack allocation
#ifdef _WIN32
#define Mem_StackAlloc(x) _alloca(x)
#else
#define Mem_StackAlloc(x) alloca(x)
#endif

#ifndef Q_MEM_DEBUG

// Normal allocation
void *		Mem_Alloc( size_t size );
void *		Mem_ReAlloc( void *block, size_t size );	// my gut tells me this causes memory fragmentation, avoid?
void *		Mem_ClearedAlloc( size_t size );
char *		Mem_CopyString( const char *in );
void		Mem_Free( void *block );

#else

#define Mem_Alloc( a ) malloc( a )
#define Mem_ReAlloc( a, b ) realloc( a, b )
#define Mem_ClearedAlloc( a ) calloc( a, 1 )
#define Mem_Free( a ) free( a )

inline char *Mem_CopyString( const char *in )
{
	char *out = (char *)Mem_Alloc( strlen( in ) + 1 );
	strcpy( out, in );
	return out;
}

#endif

// Tagged allocation
void *		Mem_TagAlloc( size_t size, uint16 tag );
void		Mem_TagFree( void *block );
void		Mem_TagFreeGroup( uint16 tag );

// Status
void		Mem_Init();
void		Mem_Shutdown();
