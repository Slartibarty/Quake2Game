/*
===================================================================================================

	Contains a thirdparty allocator implementation if applicable

===================================================================================================
*/

#include "memory_impl.h"

#ifdef Q_MEM_USE_MIMALLOC

#ifdef Q_DEBUG
#define MI_DEBUG 3
#endif

// Happy unity build

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

#include "../thirdparty/mimalloc/src/stats.c"
#include "../thirdparty/mimalloc/src/random.c"
#include "../thirdparty/mimalloc/src/os.c"
#include "../thirdparty/mimalloc/src/bitmap.c"
#include "../thirdparty/mimalloc/src/arena.c"
#include "../thirdparty/mimalloc/src/region.c"
#include "../thirdparty/mimalloc/src/segment.c"
#include "../thirdparty/mimalloc/src/page.c"
#include "../thirdparty/mimalloc/src/alloc.c"
#include "../thirdparty/mimalloc/src/alloc-aligned.c"
#include "../thirdparty/mimalloc/src/alloc-posix.c"
#include "../thirdparty/mimalloc/src/heap.c"
#include "../thirdparty/mimalloc/src/options.c"
#include "../thirdparty/mimalloc/src/init.c"

#endif
