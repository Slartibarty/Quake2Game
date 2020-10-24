//=================================================================================================
// Think Tank 2
// Copyright (c) Slartibarty. All Rights Reserved.
//=================================================================================================

#pragma once

#ifdef THINKTANK

#include "Sys_Types.h"

#else // Quake 2

#include "q_types.h"

#endif

//-------------------------------------------------------------------------------------------------
// My own implementation of a C++ vector
//
// Every time the allocation limit is reached, the buffer doubles its size, IE 512 > 1024 > 2048 > 4096
// This is the most reliable way to limit mallocs
//-------------------------------------------------------------------------------------------------

namespace tnk
{
	// Object we are representing, the amount of objects to allocate by default
	template< typename obj_t, size_t t_basealloc >
	class Vector
	{
	private:

		byte *m_Data;

	};

}
