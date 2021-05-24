//-------------------------------------------------------------------------------------------------
// Image loading routines
//-------------------------------------------------------------------------------------------------

#pragma once

#include "../../core/sys_types.h"

namespace img
{
	void	VerticalFlip( byte *image, int w, int h, int bytes_per_pixel );

	// Creates a 24-bit colormap from a provided PCX buffer
	bool	CreateColormapFromPCX( const byte *pBuffer, int nBufLen, uint *pColormap );

	// Loads a 24 or 32-bit TGA and returns a 32-bit buffer
	byte *	LoadTGA( const byte *pBuffer, int nBufLen, int &width, int &height );

	// Return true if the first 8 bytes of the buffer match the fixed PNG ID
	bool	TestPNG( const byte *buf );

	byte *	LoadPNG( byte *buf, int &width, int &height );
	bool	WritePNG( int width, int height, bool b32bit, byte *buffer, FILE *handle );
}
