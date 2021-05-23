//-------------------------------------------------------------------------------------------------
// Image loading routines
//-------------------------------------------------------------------------------------------------

#include "engine.h"

#include "png.h"

namespace img
{
	struct bgr_t
	{
		byte b, g, r;
	};

	struct bgra_t
	{
		byte b, g, r, a;
	};

	struct rgb_t
	{
		byte r, g, b;
	};

	struct rgba_t
	{
		byte r, g, b, a;
	};

	// stbi__vertical_flip
	void VerticalFlip( byte *image, int w, int h, int bytes_per_pixel )
	{
		int row;
		size_t bytes_per_row = (size_t)w * bytes_per_pixel;
		byte temp[2048];

		for ( row = 0; row < ( h >> 1 ); row++ )
		{
			byte *row0 = image + row * bytes_per_row;
			byte *row1 = image + ( h - row - 1 ) * bytes_per_row;
			// swap row0 with row1
			size_t bytes_left = bytes_per_row;
			while ( bytes_left )
			{
				size_t bytes_copy = ( bytes_left < sizeof( temp ) ) ? bytes_left : sizeof( temp );
				memcpy( temp, row0, bytes_copy );
				memcpy( row0, row1, bytes_copy );
				memcpy( row1, temp, bytes_copy );
				row0 += bytes_copy;
				row1 += bytes_copy;
				bytes_left -= bytes_copy;
			}
		}
	}

	//-------------------------------------------------------------------------------------------------

	struct PcxHeader
	{
		uint8_t		manufacturer;
		uint8_t		version;
		uint8_t		encoding;
		uint8_t		bits_per_pixel;
		uint16_t	xmin, ymin, xmax, ymax;
		uint16_t	hres, vres;
		uint8_t		palette[48];
		uint8_t		reserved;
		uint8_t		color_planes;
		uint16_t	bytes_per_line;
		uint16_t	palette_type;
		uint8_t		filler[58];
	};

	static_assert(sizeof(PcxHeader) == 128);

	//-------------------------------------------------------------------------------------------------
	// Loads an 8-bit PCX from a buffer and converts it to 32-bit
	//-------------------------------------------------------------------------------------------------
	byte *LoadPCX(const byte *pBuffer, int nBufLen, int &width, int &height)
	{
		const PcxHeader *pHeader = (const PcxHeader *)pBuffer;

		if (pHeader->manufacturer != 0x0a
			|| pHeader->version != 5
			|| pHeader->encoding != 1
			|| pHeader->bits_per_pixel != 8
			|| pHeader->xmax >= 1023
			|| pHeader->ymax >= 1023)
		{
			return nullptr;
		}

		width = pHeader->ymax + 1;
		height = pHeader->xmax + 1;

		int nNumPixels = width * height;
		int nSize = nNumPixels * 4;
		byte *pRGBA = (byte *)Mem_Alloc(nSize);				// Final RGBA image

		const byte *pData = pBuffer + sizeof(*pHeader);		// Indexes
		const byte *pPalette = pBuffer + nBufLen - 768;		// Palette map

		byte *pRGBAIter = pRGBA;

		for (int i = 0; i <= nNumPixels; ++i)
		{
			int nDataByte = *pData++;

			int nRunLength;
			if ((nDataByte & 0xC0) == 0xC0)
			{
				nRunLength = nDataByte & 0x3F;
				nDataByte = *pData++;
			}
			else
			{
				nRunLength = 1;
			}

			while (nRunLength-- > 0)
			{
				pRGBAIter[i+0] = pPalette[nDataByte + 0];
				pRGBAIter[i+1] = pPalette[nDataByte + 1];
				pRGBAIter[i+2] = pPalette[nDataByte + 2];
				pRGBAIter[i+3] = 255;
				++i;
			}
		}

		return pRGBA;
	}

	//-------------------------------------------------------------------------------------------------
	// Creates a 24-bit colormap from a provided PCX buffer
	//-------------------------------------------------------------------------------------------------
	qboolean CreateColormapFromPCX(const byte *pBuffer, int nBufLen, uint *pColormap)
	{
		const PcxHeader *pHeader = (const PcxHeader *)pBuffer;

		if (pHeader->manufacturer != 0x0a
			|| pHeader->version != 5
			|| pHeader->encoding != 1
			|| pHeader->bits_per_pixel != 8
			|| pHeader->xmax >= 1023
			|| pHeader->ymax >= 1023)
		{
			return false;
		}

		const byte *pPCXPalette = (byte *)(pBuffer + nBufLen - 768);

		for (int i = 0; i < 256; ++i)
		{
			int r = pPCXPalette[i * 3 + 0];
			int g = pPCXPalette[i * 3 + 1];
			int b = pPCXPalette[i * 3 + 2];

			int v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
			pColormap[i] = v;
		}

		pColormap[255] &= 0xffffff;	// 255 is transparent

		return true;
	}

	//-------------------------------------------------------------------------------------------------

#pragma pack(push, 1)
	struct TgaHeader
	{
		uint8_t		idlength;			// The length of a string located located after the header
		int8_t		colourmaptype;
		int8_t		datatypecode;
		int16_t		colourmaporigin;
		int16_t		colourmaplength;
		int8_t		colourmapdepth;
		int16_t		x_origin;
		int16_t		y_origin;
		int16_t		width;
		int16_t		height;
		int8_t		bitsperpixel;
		int8_t		imagedescriptor;
	};
#pragma pack(pop)

	static_assert(sizeof(TgaHeader) == 18);

	//-------------------------------------------------------------------------------------------------
	// Loads a 24 or 32-bit TGA and returns a 32-bit buffer
	//-------------------------------------------------------------------------------------------------
	byte *LoadTGA(const byte *pBuffer, int nBufLen, int &width, int &height)
	{
		const TgaHeader *pHeader = (const TgaHeader *)pBuffer;

		if (nBufLen <= sizeof(TgaHeader) || (pHeader->bitsperpixel != 24 && pHeader->bitsperpixel != 32)
			|| pHeader->datatypecode != 2 || pHeader->colourmaptype != 0)
		{
			return nullptr;
		}

		width = pHeader->width;
		height = pHeader->height;

		const byte *pData = pBuffer + sizeof(*pHeader);
		pData += pHeader->idlength;

		uint nNumPixels = (uint)width * (uint)height;
		uint nSize = nNumPixels * 4;
		byte *pPixels = (byte *)Mem_Alloc(nSize);

		byte* pPixelIter = pPixels;
		const byte* pDataIter = pData;

		if (pHeader->bitsperpixel == 24)
		{
			for (uint i = 0; i < nNumPixels; ++i, pPixelIter += 4, pDataIter += 3)
			{
				pPixelIter[0] = pDataIter[2];	// R
				pPixelIter[1] = pDataIter[1];	// G
				pPixelIter[2] = pDataIter[0];	// B
				pPixelIter[3] = 255;			// A
			}
		}
		else
		{
			for (uint i = 0; i < nNumPixels; ++i, pPixelIter += 4, pDataIter += 4)
			{
				pPixelIter[0] = pDataIter[2];	// R
				pPixelIter[1] = pDataIter[1];	// G
				pPixelIter[2] = pDataIter[0];	// B
				pPixelIter[3] = pDataIter[3];	// A
			}
		}

		// SlartTodo: there must be a better way
		VerticalFlip( pPixels, width, height, 4 );

		//stbi_write_tga("Farts.tga", width, height, 4, pPixels);
		//exit(0);

		return pPixels;
	}

	//-------------------------------------------------------------------------------------------------

	// Return true if the first 8 bytes of the buffer match the fixed PNG ID
	bool TestPNG( const byte *buf )
	{
		return *( (const int64 *)buf ) == 727905341920923785;
	}

	struct LoadPNG_UserData_t
	{
		byte *buffer;
		size_t offset;
	};

	// This callback wants us to read "length" amount of bytes into "data"
	// There must be a better way!
	static void LoadPNG_ReadCallback( png_structp png_ptr, png_bytep data, size_t length )
	{
		LoadPNG_UserData_t *userdata = (LoadPNG_UserData_t *)png_get_io_ptr( png_ptr );

		memcpy( data, userdata->buffer + userdata->offset, length );

		userdata->offset += length;
	}

	static void LoadPNG_ErrorCallback( png_structp png_ptr, png_const_charp msg )
	{
		// Load failures are bad and should be seen by everyone
		Com_FatalErrorf("PNG load error: %s\n", msg );
	}

	static void LoadPNG_WarningCallback( png_structp png_ptr, png_const_charp msg )
	{
		// Warnings are not so bad and should only be seen by devs
		Com_DPrintf( "PNG load warning: %s\n", msg );
	}

	byte *LoadPNG( byte *buf, int &width, int &height )
	{
		png_structp png_ptr;
		png_infop info_ptr;

		png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, nullptr, LoadPNG_ErrorCallback, LoadPNG_WarningCallback );
		assert( png_ptr );

		info_ptr = png_create_info_struct( png_ptr );
		assert( info_ptr );

		LoadPNG_UserData_t userdata;
		userdata.buffer = buf;
		userdata.offset = 8;

		png_set_read_fn( png_ptr, &userdata, LoadPNG_ReadCallback );
		png_set_sig_bytes( png_ptr, 8 );

		png_read_info( png_ptr, info_ptr );

		width = (int)png_get_image_width( png_ptr, info_ptr );
		height = (int)png_get_image_height( png_ptr, info_ptr );
		int color_type = png_get_color_type( png_ptr, info_ptr );
		int bit_depth = png_get_bit_depth( png_ptr, info_ptr );

		if ( color_type == PNG_COLOR_TYPE_PALETTE )
			png_set_expand( png_ptr );
		if ( color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8 )
			png_set_expand( png_ptr );
		if ( png_get_valid( png_ptr, info_ptr, PNG_INFO_tRNS ) )
			png_set_expand( png_ptr );
		if ( bit_depth == 16 )
			png_set_strip_16( png_ptr );
		if ( color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_GRAY_ALPHA )
			png_set_gray_to_rgb( png_ptr );

		int channels = png_get_channels( png_ptr, info_ptr );

		if ( channels < 4 )
		{
			png_set_filler( png_ptr, 0xFF, PNG_FILLER_AFTER );
		}

		png_read_update_info( png_ptr, info_ptr );

		size_t rowbytes = png_get_rowbytes( png_ptr, info_ptr );

		png_bytepp row_pointers = (png_bytepp)Mem_Alloc( sizeof( png_bytep ) * height );	// Row pointers
		png_bytep rows = (png_bytep)Mem_Alloc( rowbytes * height );						// Rows

		for ( int i = 0; i < height; ++i )
		{
			row_pointers[i] = rows + i * rowbytes;
		}

		png_read_image( png_ptr, row_pointers );

		Mem_Free( row_pointers ); // Don't need these anymore

		// This is optional, but calling it means we touch all data in the buffer
		//png_read_end( png_ptr, nullptr );

		png_destroy_read_struct( &png_ptr, &info_ptr, nullptr );

		return rows;
	}

	// Write a PNG using STDIO
	bool WritePNG( int width, int height, bool b32bit, byte *buffer, FILE *handle )
	{
		png_structp png_ptr;
		png_infop info_ptr;

		png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, nullptr, LoadPNG_ErrorCallback, LoadPNG_WarningCallback );
		assert( png_ptr );

		info_ptr = png_create_info_struct( png_ptr );
		assert( info_ptr );

		png_set_write_fn( png_ptr, handle, nullptr, nullptr );

		png_set_IHDR( png_ptr, info_ptr, width, height, 8, b32bit ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );

		png_write_info( png_ptr, info_ptr );

		size_t rowbytes = png_get_rowbytes( png_ptr, info_ptr );

		png_bytepp row_pointers = (png_bytepp)Mem_Alloc( sizeof( png_bytep ) * height );	// Row pointers

		for ( int i = 0; i < height; ++i )
		{
			row_pointers[i] = buffer + i * rowbytes;
		}

		png_write_image( png_ptr, row_pointers );

		Mem_Free( row_pointers );

		png_write_end( png_ptr, nullptr );

		png_destroy_write_struct( &png_ptr, &info_ptr );

		return true;
	}

}
