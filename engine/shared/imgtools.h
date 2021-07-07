//-------------------------------------------------------------------------------------------------
// Image loading routines
//-------------------------------------------------------------------------------------------------

#pragma once

#include "../../core/sys_types.h"

#ifdef _WIN32
#include <dxgiformat.h>
#else
#include "dxgiformat.h"
#endif

namespace img
{
	void	VerticalFlip( byte *image, int w, int h, int bytes_per_pixel );

	// Creates a 24-bit colormap from a provided PCX buffer
	bool	CreateColormapFromPCX( const byte *pBuffer, int nBufLen, uint *pColormap );

	// Loads a 24 or 32-bit TGA and returns a 32-bit buffer
	byte *	LoadTGA( const byte *pBuffer, int nBufLen, int &width, int &height );

	// PNG

	// Return true if the first 8 bytes of the buffer match the fixed PNG ID
	bool	TestPNG( const byte *buf );

	byte *	LoadPNG( byte *buf, int &width, int &height );
	bool	WritePNG( int width, int height, bool b32bit, byte *buffer, fsHandle_t handle );

	//-------------------------------------------------------------------------------------------------
	// DirectDraw Surface
	// https://github.com/microsoft/DirectXTex/blob/master/DirectXTex/DDS.h
	//-------------------------------------------------------------------------------------------------

#pragma pack(push, 1)

	static constexpr int32 DDS_MAGIC = MakeFourCC( 'D', 'D', 'S', ' ' );

	struct DDS_PIXELFORMAT
	{
		uint32		size;
		uint32		flags;
		uint32		fourCC;
		uint32		RGBBitCount;
		uint32		RBitMask;
		uint32		GBitMask;
		uint32		BBitMask;
		uint32		ABitMask;
	};

	#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
	#define DDS_RGB         0x00000040  // DDPF_RGB
	#define DDS_RGBA        0x00000041  // DDPF_RGB | DDPF_ALPHAPIXELS
	#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
	#define DDS_LUMINANCEA  0x00020001  // DDPF_LUMINANCE | DDPF_ALPHAPIXELS
	#define DDS_ALPHAPIXELS 0x00000001  // DDPF_ALPHAPIXELS
	#define DDS_ALPHA       0x00000002  // DDPF_ALPHA
	#define DDS_PAL8        0x00000020  // DDPF_PALETTEINDEXED8
	#define DDS_PAL8A       0x00000021  // DDPF_PALETTEINDEXED8 | DDPF_ALPHAPIXELS
	#define DDS_BUMPDUDV    0x00080000  // DDPF_BUMPDUDV

	#define DDS_HEADER_FLAGS_TEXTURE        0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
	#define DDS_HEADER_FLAGS_MIPMAP         0x00020000  // DDSD_MIPMAPCOUNT
	#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH
	#define DDS_HEADER_FLAGS_PITCH          0x00000008  // DDSD_PITCH
	#define DDS_HEADER_FLAGS_LINEARSIZE     0x00080000  // DDSD_LINEARSIZE

	#define DDS_HEIGHT 0x00000002 // DDSD_HEIGHT
	#define DDS_WIDTH  0x00000004 // DDSD_WIDTH

	#define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE
	#define DDS_SURFACE_FLAGS_MIPMAP  0x00400008 // DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
	#define DDS_SURFACE_FLAGS_CUBEMAP 0x00000008 // DDSCAPS_COMPLEX

	#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
	#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
	#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
	#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
	#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
	#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

	#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
								   DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
								   DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

	#define DDS_CUBEMAP 0x00000200 // DDSCAPS2_CUBEMAP

	#define DDS_FLAGS_VOLUME 0x00200000 // DDSCAPS2_VOLUME

	// Subset here matches D3D10_RESOURCE_DIMENSION and D3D11_RESOURCE_DIMENSION
	enum DDS_RESOURCE_DIMENSION : uint32
	{
		DDS_DIMENSION_TEXTURE1D = 2,
		DDS_DIMENSION_TEXTURE2D = 3,
		DDS_DIMENSION_TEXTURE3D = 4
	};

	// Subset here matches D3D10_RESOURCE_MISC_FLAG and D3D11_RESOURCE_MISC_FLAG
	enum DDS_RESOURCE_MISC_FLAG : uint32
	{
		DDS_RESOURCE_MISC_TEXTURECUBE = 0x4U
	};

	enum DDS_MISC_FLAGS2 : uint32
	{
		DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7U
	};

	enum DDS_ALPHA_MODE : uint32
	{
		DDS_ALPHA_MODE_UNKNOWN = 0,
		DDS_ALPHA_MODE_STRAIGHT = 1,
		DDS_ALPHA_MODE_PREMULTIPLIED = 2,
		DDS_ALPHA_MODE_OPAQUE = 3,
		DDS_ALPHA_MODE_CUSTOM = 4
	};

	struct DDS_HEADER
	{
		int32			fourCC;
		uint32			size;
		uint32			flags;
		uint32			height;
		uint32			width;
		uint32			pitchOrLinearSize;
		uint32			depth;				// only if DDS_HEADER_FLAGS_VOLUME is set in flags
		uint32			mipMapCount;
		uint32			reserved1[11];
		DDS_PIXELFORMAT	ddspf;
		uint32			caps;
		uint32			caps2;
		uint32			caps3;
		uint32			caps4;
		uint32			reserved2;
	};

	struct DDS_HEADER_DXT10
	{
		DXGI_FORMAT		dxgiFormat;
		uint32			resourceDimension;
		uint32			miscFlag;			// see D3D11_RESOURCE_MISC_FLAG
		uint32			arraySize;
		uint32			miscFlags2;			// see DDS_MISC_FLAGS2
	};

#pragma pack(pop)

	static_assert( sizeof( DDS_PIXELFORMAT ) == 32, "DDS pixel format size mismatch" );
	static_assert( sizeof( DDS_HEADER ) == 128, "DDS header size mismatch" );
	static_assert( sizeof( DDS_HEADER_DXT10 ) == 20, "DDS DX10 extended header size mismatch" );

	inline bool TestDDS( const byte *buf )
	{
		return *( (const int32 *)buf ) == DDS_MAGIC;
	}
}
