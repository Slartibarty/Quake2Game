//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#include "gl_local.h"
#include "../shared/imageloaders.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_ONLY_TGA
#define STBI_ONLY_PNG
#define STBI_NO_FAILURE_STRINGS
#include "stb_image.h"

#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

// Add to the ImageLoaders namespace
namespace ImageLoaders
{
	//------------------------------------------------------------------------------------------------
	// Loads a PCX file into an 8-bit buffer
	//------------------------------------------------------------------------------------------------
	static byte *Legacy_LoadPCX8(byte* raw, int rawlen, int& width, int& height)
	{
		pcx_t	*pcx;
		int		x, y;
		int		dataByte, runLength;
		byte	*out, *pix;
		int		xmax, ymax;

		//
		// parse the PCX file
		//
		pcx = (pcx_t*)raw;
		raw = &pcx->data;

		xmax = LittleShort(pcx->xmax);
		ymax = LittleShort(pcx->ymax);

		if (pcx->manufacturer != 0x0a
			|| pcx->version != 5
			|| pcx->encoding != 1
			|| pcx->bits_per_pixel != 8
			|| xmax >= 1023
			|| ymax >= 1023)
		{
			RI_Com_Printf("Bad pcx file (%i x %i) (%i x %i)\n", xmax + 1, ymax + 1, pcx->xmax, pcx->ymax);
			return nullptr;
		}

		width = xmax + 1;
		height = ymax + 1;

		out = (byte*)malloc(width * height);

		pix = out;

		// FIXME: use bytes_per_line here?
		for (y = 0; y <= ymax; y++, pix += xmax + 1)
		{
			for (x = 0; x <= xmax; )
			{
				dataByte = *raw++;

				if ((dataByte & 0xC0) == 0xC0)
				{
					runLength = dataByte & 0x3F;
					dataByte = *raw++;
				}
				else
					runLength = 1;

				while (runLength-- > 0)
					pix[x++] = dataByte;
			}

		}

		return out;
	}

	//------------------------------------------------------------------------------------------------
	// Loads an 8-bit PCX and converts it to 32-bit
	//------------------------------------------------------------------------------------------------
	static byte *Legacy_LoadPCX32(byte* raw, int rawlen, int& width, int& height)
	{
		byte* palette;
		byte* pic8;
		byte* pic32;

		pic8 = Legacy_LoadPCX8(raw, rawlen, width, height);
		if (!pic8) {
			return nullptr;
		}

		palette = raw + rawlen - 768;

		int c = width * height;
		pic32 = (byte*)malloc(c * 4);
		for (int i = 0; i < c; i++)
		{
			int p = pic8[i];
			pic32[0] = palette[p * 3];
			pic32[1] = palette[p * 3 + 1];
			pic32[2] = palette[p * 3 + 2];
			if (p == 255) {
				pic32[3] = 0;
			}
			else {
				pic32[3] = 255;
			}

			pic32 += 4;
		}

		free(pic8);

		return pic32;
	}

	/*
	==============
	R_LoadPCX

	Loads a PCX file
	==============
	*/
	static void R_LoadPCX(byte *raw, int rawlen, byte **pic, int &width, int &height)
	{
		pcx_t *pcx;
		int		x, y;
		int		dataByte, runLength;
		byte *out, *pix;
		int		xmax, ymax;

		*pic = NULL;
		width = 0;
		height = 0;

		//
		// parse the PCX file
		//
		pcx = (pcx_t *)raw;
		raw = &pcx->data;

		xmax = LittleShort(pcx->xmax);
		ymax = LittleShort(pcx->ymax);

		if (pcx->manufacturer != 0x0a
			|| pcx->version != 5
			|| pcx->encoding != 1
			|| pcx->bits_per_pixel != 8
			|| xmax >= 1024
			|| ymax >= 1024)
		{
			RI_Com_Printf("Bad pcx file (%i x %i) (%i x %i)\n", xmax + 1, ymax + 1, pcx->xmax, pcx->ymax);
			return;
		}

		width = xmax + 1;
		height = ymax + 1;

		out = (byte *)malloc(width * height);

		*pic = out;

		pix = out;

		// FIXME: use bytes_per_line here?
		for (y = 0; y <= ymax; y++, pix += xmax + 1)
		{
			for (x = 0; x <= xmax; )
			{
				dataByte = *raw++;

				if ((dataByte & 0xC0) == 0xC0)
				{
					runLength = dataByte & 0x3F;
					dataByte = *raw++;
				}
				else
					runLength = 1;

				while (runLength-- > 0)
					pix[x++] = dataByte;
			}

		}

		if (raw - (byte *)pcx > rawlen)
		{
			RI_Com_DPrintf("PCX file was malformed");
			free(*pic);
			*pic = NULL;
		}
	}

	/*
	==============
	LoadPCX32

	Loads an 8-bit PCX and converts it to 32-bit
	==============
	*/
	static void R_LoadPCX32(byte *raw, int rawlen, byte **pic, int &width, int &height)
	{
		byte *palette;
		byte *pic8;
		byte *pic32;

		R_LoadPCX(raw, rawlen, &pic8, width, height);
		if (!pic8) {
			*pic = NULL;
			return;
		}

		palette = raw + rawlen - 768;

		int c = width * height;
		pic32 = *pic = (byte *)malloc(4 * c);
		for (int i = 0; i < c; i++)
		{
			int p = pic8[i];
			pic32[0] = palette[p * 3];
			pic32[1] = palette[p * 3 + 1];
			pic32[2] = palette[p * 3 + 2];
			if (p == 255) {
				pic32[3] = 0;
			}
			else {
				pic32[3] = 255;
			}

			pic32 += 4;
		}

		free(pic8);
	}

	//-------------------------------------------------------------------------------------------------
	// Loads a WAL
	//-------------------------------------------------------------------------------------------------
	byte *LoadWAL(const byte *pBuffer, int nBufLen, int &width, int &height)
	{
		const miptex_t *pMipTex = (const miptex_t *)pBuffer;

		if (nBufLen <= sizeof(*pMipTex))
		{
			return nullptr;
		}

		width = (int)pMipTex->width;
		height = (int)pMipTex->height;

		int c = width * height;

		byte *pPic8 = (byte *)pMipTex + pMipTex->offsets[0];
		uint *pPic32 = (uint *)malloc(c * 4);

		for (int i = 0; i < c; ++i)
		{
			pPic32[i] = d_8to24table[pPic8[i]];
		}

		return (byte *)pPic32;
	}
}

//-------------------------------------------------------------------------------------------------

material_t *mat_notexture; // use for bad textures
material_t *mat_particletexture; // little dot for particles

image_t		gltextures[MAX_GLTEXTURES];
int			numgltextures;
material_t	glmaterials[MAX_GLMATERIALS];
int			numglmaterials;

byte		g_gammatable[256];

unsigned	d_8to24table[256];

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// List textures
//-------------------------------------------------------------------------------------------------
void GL_ImageList_f( void )
{
#if 0
	int i;
	image_t *image;
	size_t texels;

	RI_Com_Printf( "------------------\n" );
	texels = 0;

	for ( i = 0, image = gltextures; i < numgltextures; i++, image++ )
	{
		assert( image->texnum > 0 );
	//	if ( image->texnum <= 0 )
	//		continue;
		texels += (size_t)image->width * (size_t)image->height;
		switch ( image->type )
		{
		case it_skin:
			RI_Com_Printf( "M" );
			break;
		case it_sprite:
			RI_Com_Printf( "S" );
			break;
		case it_wall:
			RI_Com_Printf( "W" );
			break;
		case it_pic:
			RI_Com_Printf( "P" );
			break;
		case it_sky:
			RI_Com_Printf( "E" );
			break;
		default:
			RI_Com_Printf( " " );
			break;
		}

		RI_Com_Printf( " %3d %3d RGBA: %s\n", image->width, image->height, image->name );
	}

	RI_Com_Printf( "Total texel count (not counting mipmaps): %zu\n", texels );
#else
	RI_Com_Printf( "NOT IMPLEMENTED!\n" );
#endif
}

//-------------------------------------------------------------------------------------------------
// List materials
//-------------------------------------------------------------------------------------------------
/*void GL_MaterialList_f()
{
	int i;
	material_t *material;

	RI_Com_Printf( "------------------\n" );

	for ( i = 0, material = glmaterials; i < numgltextures; ++i, ++material )
	{
		RI_Com_Printf( "%3d %3d RGB: %s\n", image->width, image->height, material->name );
	}
}*/

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// This must NEVER fail
//
// Only mipmap select types
// We want to mipmap walls, skins and sprites, although never
// 2D pics or skies
//-------------------------------------------------------------------------------------------------
static GLuint GL_Upload32( const byte *pData, int nWidth, int nHeight, imageflags_t flags )
{
	GLuint id;

	// Generate and bind the texture
	glGenTextures( 1, &id );
	GL_Bind( id );

	// Upload it
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pData );

	// Mips
	if ( !( flags & IF_NOMIPS ) )
	{
		if ( GLEW_ARB_framebuffer_object )
		{
			// This sucks
			glGenerateMipmap( GL_TEXTURE_2D );
		}
		else
		{
			// This sucks even more
			int nOutWidth = nWidth / 2;
			int nOutHeight = nHeight / 2;
			int nLevel = 1;

			byte *pNewData = (byte *)malloc( ( nOutWidth * nOutHeight ) * 4 );

			while ( nOutWidth > 1 && nOutHeight > 1 )
			{
				stbir_resize_uint8( pData, nWidth, nHeight, 0, pNewData, nOutWidth, nOutHeight, 0, 4 );
				glTexImage2D( GL_TEXTURE_2D, nLevel, GL_RGBA8, nOutWidth, nOutHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pNewData );

				++nLevel;
				nOutWidth /= 2;
				nOutHeight /= 2;
			}

			free( pNewData );
		}

		if ( flags & IF_NEAREST )
		{
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		}
		else
		{
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		}

		if ( GLEW_ARB_texture_filter_anisotropic && !( flags & IF_NOANISO ) )
		{
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 16.0f ); // Don't hardcode this value
		}
	}
	else
	{
		// No mips
		if ( flags & IF_NEAREST )
		{
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		}
		else
		{
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		}
	}

	// Clamp
	if ( GLEW_EXT_texture_edge_clamp && ( flags & IF_CLAMPS ) )
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE_EXT );
	}
	if ( GLEW_EXT_texture_edge_clamp && ( flags & IF_CLAMPT ) )
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE_EXT );
	}

	return id;
}

//-------------------------------------------------------------------------------------------------
// This is the only function that can create image_t's
//-------------------------------------------------------------------------------------------------
static image_t *GL_CreateImage(const char *name, const byte *pic, int width, int height, imageflags_t flags)
{
	int			i;
	image_t		*image;

	// find a free image_t
	for (i = 0, image = gltextures; i < numgltextures; i++, image++)
	{
		if (!image->texnum)
			break;
	}
	if (i == numgltextures)
	{
		if (numgltextures == MAX_GLTEXTURES)
			RI_Com_Error(ERR_DROP, "MAX_GLTEXTURES");
		numgltextures++;
	}
	image = &gltextures[i];

	Q_strcpy_s(image->name, name);
	image->refcount = 0;			// Incremented by GL_FindImage

	image->width = width;
	image->height = height;
	image->flags = flags;

	// Upload it!
	// Pics and skies are never mipmapped (they don't need em)
	image->texnum = GL_Upload32(pic, width, height, flags);

	image->sl = 0;
	image->sh = 1;
	image->tl = 0;
	image->th = 1;

	return image;
}

//-------------------------------------------------------------------------------------------------
// Loads any of the supported image types into a cannonical 32 bit format.
//-------------------------------------------------------------------------------------------------
static byte *GL_LoadImage(const char *pName, int &width, int &height)
{
	byte *pBuffer;
	int nBufLen;

	nBufLen = RI_FS_LoadFile(pName, (void **)&pBuffer);
	if (!pBuffer)
	{
		return nullptr;
	}

	byte *pPic;

	const char *pExt = strrchr(pName, '.') + 1;

	if (Q_stricmp(pExt, "png") == 0 || Q_stricmp(pExt, "tga") == 0)
	{
		pPic = stbi_load_from_memory(pBuffer, nBufLen, &width, &height, nullptr, 4);
	}
	else if (Q_stricmp(pExt, "pcx") == 0)
	{
		// Fixme
		ImageLoaders::R_LoadPCX32(pBuffer, nBufLen, &pPic, width, height);
	}
	else if (Q_stricmp(pExt, "wal") == 0)
	{
		pPic = ImageLoaders::LoadWAL(pBuffer, nBufLen, width, height);
	}
	else
	{
		RI_FS_FreeFile(pBuffer);
		RI_Com_Printf("GL_LoadImage - %s is an unsupported image format!", pName);
		return nullptr;
	}

	RI_FS_FreeFile( pBuffer );

	if (!pPic)
	{
		return nullptr;
	}

	return pPic;
}

struct rgb_t
{
	byte r, g, b;
};

//-------------------------------------------------------------------------------------------------
// Finds or loads the given image
//
// THIS CHOKES ON TEXTURES WITH THE SAME NAME, STUDIOMODELS CAN HAVE IDENTICAL TEXTURE NAMES THAT ARE
// DIFFERENT!!! THIS IS BAD!
//-------------------------------------------------------------------------------------------------
image_t	*GL_FindImage (const char *name, imageflags_t flags, byte *pic = nullptr, int width = 0, int height = 0)
{
	int i, j;
	image_t *image;

	assert( name && name[0] );

	// look for it
	for ( i = 0, image = gltextures; i < numgltextures; ++i, ++image )
	{
		if ( Q_strcmp( name, image->name ) == 0 )
		{
			++image->refcount;
			return image;
		}
	}

	//
	// load the pic from an 8-bit buffer (studiomodels)
	//
	if ( pic )
	{
		int outsize = width * height * 4;
		byte *pic32 = (byte *)malloc( outsize );
		rgb_t *palette = (rgb_t *)( pic + width * height );

		for ( i = 0, j = 0; i < outsize; i += 4, ++j )
		{
			pic32[i+0] = palette[pic[j]].r;
			pic32[i+1] = palette[pic[j]].g;
			pic32[i+2] = palette[pic[j]].b;
			pic32[i+3] = 255; // SlartTodo: Studiomodels can have alphatest transparency
		}
		
		image = GL_CreateImage( name, pic32, width, height, flags );

		free( pic32 );

		++image->refcount;
		return image;
	}

	//
	// load the pic from disk
	//
	pic = GL_LoadImage( name, width, height );
	if ( !pic ) {
		mat_notexture->image->IncrementRefCount();
		return mat_notexture->image;
	}

	image = GL_CreateImage( name, pic, width, height, flags );

	free( pic );

	++image->refcount;
	return image;
}

#define FLAGCHECK(flag) if ( Q_strcmp( data, #flag ) == 0 ) { return flag; }

int StringToContentFlag( const char *data )
{
	FLAGCHECK( CONTENTS_SOLID );
	FLAGCHECK( CONTENTS_WINDOW );
	FLAGCHECK( CONTENTS_AUX );
	FLAGCHECK( CONTENTS_LAVA );
	FLAGCHECK( CONTENTS_SLIME );
	FLAGCHECK( CONTENTS_WATER );
	FLAGCHECK( CONTENTS_MIST );
	// last visible
	FLAGCHECK( CONTENTS_AREAPORTAL );
	FLAGCHECK( CONTENTS_PLAYERCLIP );
	FLAGCHECK( CONTENTS_MONSTERCLIP );
	FLAGCHECK( CONTENTS_CURRENT_0 );
	FLAGCHECK( CONTENTS_CURRENT_90 );
	FLAGCHECK( CONTENTS_CURRENT_180 );
	FLAGCHECK( CONTENTS_CURRENT_270 );
	FLAGCHECK( CONTENTS_CURRENT_UP );
	FLAGCHECK( CONTENTS_CURRENT_DOWN );
	FLAGCHECK( CONTENTS_ORIGIN );
	FLAGCHECK( CONTENTS_MONSTER );
	FLAGCHECK( CONTENTS_DEADMONSTER );
	FLAGCHECK( CONTENTS_DETAIL );
	FLAGCHECK( CONTENTS_TRANSLUCENT );
	FLAGCHECK( CONTENTS_LADDER );

	return 0;
}

int StringToSurfaceFlag( const char *data )
{
	FLAGCHECK( SURF_LIGHT );
	FLAGCHECK( SURF_SLICK );
	FLAGCHECK( SURF_SKY );
	FLAGCHECK( SURF_WARP );
	FLAGCHECK( SURF_TRANS33 );
	FLAGCHECK( SURF_TRANS66 );
	FLAGCHECK( SURF_FLOWING );
	FLAGCHECK( SURF_NODRAW );
	FLAGCHECK( SURF_HINT );
	FLAGCHECK( SURF_SKIP );

	return 0;
}

#undef FLAGCHECK

int ParseSurfaceFlags( char *data, int type )
{
	char tokenhack[MAX_TOKEN_CHARS];
	char *token = tokenhack;
	int flags = 0;

	COM_Parse2( &data, &token, sizeof( tokenhack ) );

	// Parse until the end baby
	while ( data )
	{
		if ( type == 1 )
			flags |= StringToContentFlag( token );
		else
			flags |= StringToSurfaceFlag( token );

		COM_Parse2( &data, &token, sizeof( tokenhack ) );
	}

	return flags;
}

bool ParseMaterial( char *data, material_t *material )
{
	char tokenhack[MAX_TOKEN_CHARS];
	char *token = tokenhack;

	char basetexture[MAX_TOKEN_CHARS]; // For storing off the basetexture for later
	basetexture[0] = '\0';

	imageflags_t flags = 0;

	COM_Parse2( &data, &token, sizeof( tokenhack ) );
	if ( token[0] != '{' )
	{
		// Malformed
		RI_Com_Printf( "Malformed material %s\n", material->name );
		return false;
	}

	COM_Parse2( &data, &token, sizeof( tokenhack ) );

	for ( ; token[0] != '}'; COM_Parse2( &data, &token, sizeof( tokenhack ) ) )
	{
		if ( Q_strcmp( token, "$basetexture" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			Q_strcpy_s( basetexture, token );
			continue;
		}
		if ( Q_strcmp( token, "$nextframe" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			material->nextframe = GL_FindMaterial( token );
			if ( material->nextframe == mat_notexture ) // SLARTHACK: THIS IS REALLY BAD!!! REFCOUNT!!!
			{
				material->nextframe = nullptr;
			}
			continue;
		}
		if ( Q_strcmp( token, "$nomips" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			if ( atoi( token ) )
			{
				flags |= IF_NOMIPS;
			}
			continue;
		}
		if ( Q_strcmp( token, "$noaniso" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			if ( atoi( token ) )
			{
				flags |= IF_NOANISO;
			}
			continue;
		}
		if ( Q_strcmp( token, "$nearestfilter" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			if ( atoi( token ) )
			{
				flags |= IF_NEAREST;
			}
			continue;
		}
		if ( Q_strcmp( token, "$clamps" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			if ( atoi( token ) )
			{
				flags |= IF_CLAMPS;
			}
			continue;
		}
		if ( Q_strcmp( token, "$clampt" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			if ( atoi( token ) )
			{
				flags |= IF_CLAMPT;
			}
			continue;
		}
		if ( Q_strcmp( token, "$contentflags" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			material->contentflags = ParseSurfaceFlags( token, 1 );
			continue;
		}
		if ( Q_strcmp( token, "$surfaceflags" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			material->surfaceflags = ParseSurfaceFlags( token, 0 );
			continue;
		}
		if ( Q_strcmp( token, "$value" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			material->value = atoi( token );
			continue;
		}
	}

	if ( basetexture[0] )
	{
		material->image = GL_FindImage( basetexture, flags );
	}

	return true;
}

static material_t *GL_CreateMaterialFromData( const char *name, image_t *image )
{
	int i;
	material_t *material;

	// find a free material_t
	for ( i = 0, material = glmaterials; i < numglmaterials; i++, material++ )
	{
		if ( material->image == nullptr )
			break;
	}
	if ( i == numglmaterials )
	{
		if ( numglmaterials == MAX_GLMATERIALS )
			RI_Com_Error( ERR_DROP, "MAX_GLMATERIALS" );
		++numglmaterials;
	}
	material = &glmaterials[i];
	memset( material, 0, sizeof( *material ) );
	
	Q_strcpy_s( material->name, name );
	material->image = image;
	++material->image->refcount;

	return material;
}

static material_t *GL_CreateMaterial( const char *name )
{
	char *pBuffer;
	int nBufLen;
	int i;
	material_t *material;

	// Horrible hack, clean this up
#if 0
	char newname[MAX_QPATH];
	assert( strlen( name ) < MAX_QPATH );
	Q_strcpy_s( newname, name );
	strcat( newname, ".mat" );
#endif

	nBufLen = RI_FS_LoadFile( name, (void **)&pBuffer, 1 );
	if ( !pBuffer )
	{
		return mat_notexture;
	}

	// find a free material_t
	for ( i = 0, material = glmaterials; i < numglmaterials; i++, material++ )
	{
		if ( material->image == nullptr )
			break;
	}
	if ( i == numglmaterials )
	{
		if ( numglmaterials == MAX_GLMATERIALS )
			RI_Com_Error( ERR_DROP, "MAX_GLMATERIALS" ); // This can probably just return mat_notexture
		++numglmaterials;
	}
	material = &glmaterials[i];
	memset( material, 0, sizeof( *material ) );

	Q_strcpy_s( material->name, name );
	material->image = mat_notexture->image;

	if ( !ParseMaterial( pBuffer, material ) )
	{
		return mat_notexture;
	}

	return material;
}

material_t *GL_FindMaterial( const char *name, byte *pic, int width, int height )
{
	int i;
	material_t *material;

	assert( name && name[0] );

	if ( !strstr( name, ".mat" ) )
	{
		return mat_notexture;
	}

	// look for it
	for ( i = 0, material = glmaterials; i < numglmaterials; ++i, ++material )
	{
		if ( Q_strcmp( name, material->name ) == 0 )
		{
			material->registration_sequence = registration_sequence;
			return material;
		}
	}

	material = GL_CreateMaterial( name );

	return material;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
material_t *R_RegisterSkin( const char *name )
{
	// No
	return mat_notexture;
}

#if 0
//-------------------------------------------------------------------------------------------------
// Any image that was not touched on this registration sequence
// will be freed.
//-------------------------------------------------------------------------------------------------
void GL_FreeUnusedImages( void )
{
	int i;
	image_t *image;

	for ( i = 0, image = gltextures; i < numgltextures; i++, image++ )
	{
		if ( image->refcount != 0 )
			continue;		// used this sequence

		if ( image == mat_notexture->image || image == mat_particletexture->image )
			continue;		// generated texture, delete only at shutdown

		image->Delete();
	}
}
#endif

//-------------------------------------------------------------------------------------------------
// Any image that was not touched on this registration sequence
// will be freed.
//-------------------------------------------------------------------------------------------------
void GL_FreeUnusedMaterials( void )
{
	int i;
	material_t *material;

	for ( i = 0, material = glmaterials; i < numglmaterials; i++, material++ )
	{
		if ( material == mat_notexture || material == mat_particletexture )
			continue;		// generated material, delete only at shutdown
		if ( material->registration_sequence == registration_sequence )
			continue;		// used this sequence
		if ( material->registration_sequence == 0 )
			continue;		// free image_t slot

		material->Delete();
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void GL_GetPalette (void)
{
	byte	*pBuffer;
	int		nBufLen;

	nBufLen = RI_FS_LoadFile("pics/colormap.pcx", (void**)&pBuffer);
	if (!pBuffer)
	{
		RI_Com_Error(ERR_FATAL, "Couldn't load pics/colormap.pcx");
	}

	if (!ImageLoaders::CreateColormapFromPCX(pBuffer, nBufLen, d_8to24table))
	{
		RI_FS_FreeFile(pBuffer);
		RI_Com_Error(ERR_FATAL, "pics/colormap.pcx is not a valid PCX!");
	}

	RI_FS_FreeFile(pBuffer);
}

//-------------------------------------------------------------------------------------------------
// Builds the gamma table
//-------------------------------------------------------------------------------------------------
static void GL_BuildGammaTable( float gamma, int overbright )
{
	int i, inf;

	for ( i = 0; i < 256; ++i )
	{
		inf = static_cast<int>( 255.0f * powf( i / 255.0f, 1.0f / gamma ) + 0.5f );

		// Apply overbrightening
		inf <<= overbright;

		// Clamp value
		inf = Clamp( inf, 0, 255 );

		g_gammatable[i] = static_cast<byte>( inf );
	}
}

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Create the default texture
// also used when pointparameters aren't supported
//-------------------------------------------------------------------------------------------------
static const byte particletexture[8][8]{
	{0,0,0,0,0,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
};

static const byte missingtexture[8][8]{
	{1,1,0,0,1,1,0,0},
	{1,1,0,0,1,1,0,0},
	{0,0,1,1,0,0,1,1},
	{0,0,1,1,0,0,1,1},
	{1,1,0,0,1,1,0,0},
	{1,1,0,0,1,1,0,0},
	{0,0,1,1,0,0,1,1},
	{0,0,1,1,0,0,1,1}
};

static void R_InitParticleTexture(void)
{
	int x, y;
	byte data[8][8][4];
	image_t *img;

	//
	// particle texture
	//
	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = particletexture[x][y] * 255;
		}
	}
	img = GL_CreateImage("***particle***", (byte *)data, 8, 8, IF_NOANISO | IF_CLAMPS | IF_CLAMPT);
	mat_particletexture = GL_CreateMaterialFromData( "***particle***", img );

	//
	// also use this for bad textures, but without alpha
	//
	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			data[y][x][0] = missingtexture[x][y] * 255;
			data[y][x][1] = 0;
			data[y][x][2] = missingtexture[x][y] * 255;
			data[y][x][3] = 255;
		}
	}
	img = GL_CreateImage("***r_notexture***", (byte *)data, 8, 8, IF_NONE);
	mat_notexture = GL_CreateMaterialFromData( "***r_notexture***", img );
	// We upload as a pic, but we don't want clamping! Set it here
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void GL_InitImages(void)
{
	registration_sequence = 1;

	GL_GetPalette();

	GL_BuildGammaTable( vid_gamma->value, 2 );

	R_InitParticleTexture();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void GL_ShutdownImages (void)
{
	int i;
	material_t *material;
	image_t *image;

	// Clear our generated materials
	extern material_t *draw_chars;
	draw_chars->Delete();
	mat_particletexture->Delete();
	mat_notexture->Delete();

	// Clear materials
	for ( i = 0, material = glmaterials; i < numglmaterials; ++i, ++material )
	{
		if ( material->image == nullptr )
			continue; // Free slot

		// Decrement refcount
		material->Delete();
	}

	// Images are dereferenced by the material when their refcount reaches 0
	// Go through every single material for security
#ifdef Q_DEBUG
	for ( i = 0, image = gltextures; i < MAX_GLTEXTURES; ++i, ++image )
	{
		assert( image->refcount == 0 && image->texnum == 0 );
	}
#endif
}

//-------------------------------------------------------------------------------------------------
// Helpers
//-------------------------------------------------------------------------------------------------

void GL_EnableMultitexture(qboolean enable)
{
	if (!GLEW_ARB_multitexture || !gl_ext_multitexture->value)
		return;

	if (enable)
	{
		GL_SelectTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		GL_TexEnv(GL_REPLACE);
	}
	else
	{
		GL_SelectTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		GL_TexEnv(GL_REPLACE);
	}
	GL_SelectTexture(GL_TEXTURE0);
	GL_TexEnv(GL_REPLACE);
}

void GL_SelectTexture(GLenum texture)
{
	if (!GLEW_ARB_multitexture || !gl_ext_multitexture->value)
		return;

	int tmu;

	if (texture == GL_TEXTURE0)
	{
		tmu = 0;
	}
	else
	{
		tmu = 1;
	}

	if (tmu == gl_state.currenttmu)
	{
		return;
	}

	gl_state.currenttmu = tmu;

	glActiveTextureARB(texture);
	glClientActiveTextureARB(texture);
}

void GL_TexEnv(GLint mode)
{
	static GLint lastmodes[2] = { -1, -1 };

	if (mode != lastmodes[gl_state.currenttmu])
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
		lastmodes[gl_state.currenttmu] = mode;
	}
}

void GL_Bind( GLuint texnum )
{
	// Are we already bound?
	if ( gl_state.currenttextures[gl_state.currenttmu] == texnum )
		return;

	gl_state.currenttextures[gl_state.currenttmu] = texnum;
	glBindTexture( GL_TEXTURE_2D, texnum );
}

void GL_MBind(GLenum target, GLuint texnum)
{
	GL_SelectTexture(target);
	if (target == GL_TEXTURE0)
	{
		if (gl_state.currenttextures[0] == texnum)
			return;
	}
	else
	{
		if (gl_state.currenttextures[1] == texnum)
			return;
	}
	GL_Bind(texnum);
}
