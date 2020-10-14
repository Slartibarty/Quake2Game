//-------------------------------------------------------------------------------------------------
// Image loading routines
//-------------------------------------------------------------------------------------------------

#include <cstdlib>
#include <cstdint>
#include <cassert>

#include "../../common/q_types.h"

namespace ImageLoaders
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
		byte *pRGBA = (byte *)malloc(nSize);				// Final RGBA image

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
		byte *pPixels = (byte *)malloc(nSize);

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

		//stbi_write_tga("Farts.tga", width, height, 4, pPixels);
		//exit(0);

		return pPixels;
	}

#if 0

	typedef byte *(*LoadFunc)(const byte *pBuffer, int nBufLen, int &width, int &height);

	static const LoadFunc s_ImageLoaders[]
	{
		LoadTGA,
		LoadPCX
	};

#undef LoadImage	// Windows.h

	//-------------------------------------------------------------------------------------------------
	// Goes through all image loaders until something works
	//-------------------------------------------------------------------------------------------------
	byte *LoadImage(const byte *pBuffer, int nBufLen, int &width, int &height)
	{
		byte *result;

		for (int i = 0; i < countof(s_ImageLoaders); ++i)
		{
			result = s_ImageLoaders[i](pBuffer, nBufLen, width, height);
			if (result)
			{
				return result;
			}
		}

		return nullptr;
	}

#endif

}
