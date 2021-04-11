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

// Normal allocation
void *		Mem_Alloc( size_t size );
void *		Mem_ReAlloc( void *block, size_t size );	// my gut tells me this causes memory fragmentation, avoid?
void *		Mem_ClearedAlloc( size_t size );
char *		Mem_CopyString( const char *in );
void		Mem_Free( void *block );

// Tagged allocation
void *		Mem_TagAlloc( size_t size, uint16 tag );
void		Mem_TagFree( void *block );
void		Mem_TagFreeGroup( uint16 tag );

// Status
void		Mem_Init();
void		Mem_Shutdown();

#define Z_StackAlloc Mem_StackAlloc

#define Z_Malloc Mem_Alloc
#define Z_Realloc Mem_ReAlloc
#define Z_Calloc Mem_ClearedAlloc
#define Z_CopyString Mem_CopyString
#define Z_Free Mem_Free

#define Z_TagMalloc Mem_TagAlloc
#define Z_TagFree Mem_TagFree
#define Z_TagFreeGroup Mem_TagFreeGroup

#define Z_Init Mem_Init
#define Z_Shutdown Mem_Shutdown
