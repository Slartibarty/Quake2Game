//-------------------------------------------------------------------------------------------------
// Image loading routines
//-------------------------------------------------------------------------------------------------

#pragma once

#include "../../common/q_types.h"

namespace ImageLoaders
{
	// Returns a 32-bit buffer (8-bit PCX only, 24/32-bit TGA only)
	byte		*LoadPCX(const byte *pBuffer, int nBufLen, int &width, int &height);
	byte		*LoadTGA(const byte *pBuffer, int nBufLen, int &width, int &height);

	// Creates a 24-bit colormap from a provided PCX buffer
	qboolean	CreateColormapFromPCX(const byte *pBuffer, int nBufLen, uint *pColormap);
}
