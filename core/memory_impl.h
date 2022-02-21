
#pragma once

// FIXME: Use mimalloc on Linux too
#if !defined Q_DEBUG && defined _WIN32
//#define Q_MEM_USE_MIMALLOC
#endif
