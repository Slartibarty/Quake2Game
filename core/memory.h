/*
===================================================================================================

	Memory management

===================================================================================================
*/

#pragma once

#include <cstddef>			// size_t
#include "sys_defines.h"	// RESTRICTFN
#include "sys_types.h"		// uint16

// Stack allocation
#ifdef _WIN32
#include <malloc.h>
#define Mem_StackAlloc(x) _alloca(x)
#else
#include <alloca.h>
#define Mem_StackAlloc(x) alloca(x)
#endif

#ifndef Q_MEM_DEBUG

// Normal allocation
[[nodiscard]] RESTRICTFN void *	Mem_Alloc( size_t size );
[[nodiscard]] RESTRICTFN void *	Mem_ReAlloc( void *block, size_t size );	// my gut tells me this causes memory fragmentation, avoid?
[[nodiscard]] RESTRICTFN void *	Mem_ClearedAlloc( size_t size );
[[nodiscard]] RESTRICTFN char *	Mem_CopyString( const char *in );
[[nodiscard]] size_t			Mem_Size( void *block );
void							Mem_Free( void *block );

#else

// mem debug is never active on Linux

#define Mem_Alloc( a ) malloc( a )
#define Mem_ReAlloc( a, b ) realloc( a, b )
#define Mem_ClearedAlloc( a ) calloc( 1, a )
#define Mem_CopyString( a ) _strdup( a )
#define Mem_Size( a ) _msize( a )
#define Mem_Free( a ) free( a )

#endif

// Tagged allocation
[[nodiscard]] RESTRICTFN void *	Mem_TagAlloc( size_t size, uint16 tag );
void							Mem_TagFree( void *block );
void							Mem_TagFreeGroup( uint16 tag );

// Status
void		Mem_Init();
void		Mem_Shutdown();
