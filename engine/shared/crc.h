// crc.h

#pragma once

#include "../../core/sys_types.h"

namespace crc16
{
	void Init( uint16 &crcvalue );
	void ProcessByte( uint16 &crcvalue, byte data );
	uint16 Value( uint16 crcvalue );
	uint16 Block( byte *start, int count );
}
