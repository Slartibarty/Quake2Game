//-------------------------------------------------------------------------------------------------
// Image and material handling
// 
// Images are ref-counted and can be shared amongst many materials
// Materials are managed like images used to be, with the registration_sequence
// A registration_sequence of -1 means the material is managed manually, rather than being cleared at
// every R_EndRegistration, this allows for mats/imgs like the conback and conchars to be loaded at all times
// without being re-loaded constantly
//-------------------------------------------------------------------------------------------------

#include "gl_local.h"
#include "../shared/imgtools.h"

#if 0
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STBIR_MAX_CHANNELS 32
#define STBIR_MALLOC(size,c) ((void)(c), Mem_Alloc(size))
#define STBIR_FREE(ptr,c) ((void)(c), Mem_Free(ptr))
#include "stb_image_resize.h"
#endif

//-------------------------------------------------------------------------------------------------

material_t *	defaultMaterial;
material_t *	blackMaterial;
material_t *	whiteMaterial;
image_t *		flatNormalImage;

image_t		gltextures[MAX_GLTEXTURES];
int			numgltextures;
material_t	glmaterials[MAX_GLMATERIALS];
int			numglmaterials;

byte		g_gammatable[256];

unsigned	d_8to24table[256];

// Are we initialised?
static bool g_imagesInitialised;

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

	Com_Printf( "------------------\n" );
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
			Com_Printf( "M" );
			break;
		case it_sprite:
			Com_Printf( "S" );
			break;
		case it_wall:
			Com_Printf( "W" );
			break;
		case it_pic:
			Com_Printf( "P" );
			break;
		case it_sky:
			Com_Printf( "E" );
			break;
		default:
			Com_Printf( " " );
			break;
		}

		Com_Printf( " %3d %3d RGBA: %s\n", image->width, image->height, image->name );
	}

	Com_Printf( "Total texel count (not counting mipmaps): %zu\n", texels );
#else
	Com_Printf( "NOT IMPLEMENTED!\n" );
#endif
}

//-------------------------------------------------------------------------------------------------
// List materials
//-------------------------------------------------------------------------------------------------
void GL_MaterialList_f()
{
	int i, managedCount = 0;
	material_t *material;

	Com_Print( "-------------------------------------------------------------------------------\n" );

	for ( i = 0, material = glmaterials; i < numglmaterials; ++i, ++material )
	{
		Com_Printf( "Material: %s\nregistration_sequence = %d\n", material->name, material->registration_sequence );
		if ( material->registration_sequence == -1 ) {
			++managedCount;
		}
	}

	Com_Printf( "\n%d registered materials\n%d of those are managed\n", numglmaterials, managedCount );

	Com_Print( "-------------------------------------------------------------------------------\n" );
}

//-------------------------------------------------------------------------------------------------

static void GL_ApplyTextureParameters( imageFlags_t flags )
{
	//flags |= IF_NEAREST;

	// Mips
	if ( !( flags & IF_NOMIPS ) )
	{
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
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 16.0f ); // don't hardcode this value
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
}

// for compressed images, pData is the whole file
// this function is not allowed to fail under any circumstances
static GLuint GL_UploadCompressed( const byte *pBuffer, imageFlags_t flags )
{
	GLuint id;

	glGenTextures( 1, &id );
	GL_BindTexture( id );

	// by this point we know several things are for sure:
	//  1. DDPF_FOURCC is present in ddspf
	//  2. We have either DXT1, DXT1a, DXT3 or DXT5
	//  3. The file is longer than 128 bytes, or 128 + 20 if we have the DX10 extensions

	const img::DDS_HEADER *pHeader = (const img::DDS_HEADER *)pBuffer;

	assert( pHeader->mipMapCount >= 1 );

	GLint mipCount = (GLint)pHeader->mipMapCount;

	if ( !( pHeader->flags & DDS_HEADER_FLAGS_MIPMAP ) )
	{
		// no mips?
	}

	GLsizei width = (GLsizei)pHeader->width;
	GLsizei height = (GLsizei)pHeader->height;

	GLenum format;
	int blockSize;

	const img::DDS_HEADER_DXT10 *pHeader2 = (const img::DDS_HEADER_DXT10 *)( pBuffer + sizeof( img::DDS_HEADER ) );

	switch ( pHeader2->dxgiFormat )
	{
		// Classic DXT1
	case DXGI_FORMAT_BC1_UNORM:
		format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		blockSize = 8;
		break;
		// New BC4 and BC5
	case DXGI_FORMAT_BC4_UNORM:
		format = GL_COMPRESSED_RED_RGTC1;
		blockSize = 8;
		break;
	case DXGI_FORMAT_BC5_UNORM:
		format = GL_COMPRESSED_RG_RGTC2;
		blockSize = 16;
		break;
		// Super mega new (10 years old) BC6 and BC7
	case DXGI_FORMAT_BC7_UNORM:
		format = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
		blockSize = 16;
		break;
	default:
		Com_FatalError( "A DDS file was present with a compression type not yet supported.\nSorry for crashing but I don't want to refactor the code just yet.\n" );
	}

	// mip 0
	const byte *pMip = pBuffer + sizeof( img::DDS_HEADER ) + sizeof( img::DDS_HEADER_DXT10 );

	for ( GLint level = 0; level < mipCount; ++level )
	{
		GLsizei mipSize = Max( 1, ( ( width + 3 ) / 4 ) ) * Max( 1, ( ( height + 3 ) / 4 ) ) * blockSize;

		glCompressedTexImage2D( GL_TEXTURE_2D, level, format, width, height, 0, mipSize, pMip );

		width = Max( 1, width / 2 );
		height = Max( 1, height / 2 );

		// add the size of the mip to get the offset to the next mip
		pMip += mipSize;
	}

	GL_ApplyTextureParameters( flags );

	return id;
}

// uploads a 32-bit, uncompressed texture pointed to by pData
static GLuint GL_Upload( const byte *pData, int width, int height, imageFlags_t flags )
{
	GLuint id;

	glGenTextures( 1, &id );
	GL_BindTexture( id );

	// Sad... Need to fixup SRGB stuff eventually
	GLint internalFormat = ( flags & IF_SRGB ) ? GL_SRGB8_ALPHA8 : GL_RGBA8;

	glTexImage2D( GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pData );

	// auto-generate mipmaps (FIXME: sad vendor-specific extension)
	if ( GLEW_ARB_framebuffer_object )
	{
		glGenerateMipmap( GL_TEXTURE_2D );
	}

	GL_ApplyTextureParameters( flags );

	return id;
}

//-------------------------------------------------------------------------------------------------
// This is the only function that can create image_t's
//-------------------------------------------------------------------------------------------------
static image_t *GL_CreateImage( const char *name, byte *pic, int width, int height, imageFlags_t flags, bool compressed )
{
	int			i;
	image_t *	image;

	// find a free image_t
	for ( i = 0, image = gltextures; i < numgltextures; i++, image++ )
	{
		if ( !image->texnum ) {
			break;
		}
	}
	if ( i == numgltextures )
	{
		if ( numgltextures == MAX_GLTEXTURES ) {
			Com_Error( "MAX_GLTEXTURES" );
		}
		numgltextures++;
	}
	image = &gltextures[i];

	Q_strcpy_s( image->name, name );
	image->refcount = 0;			// Incremented by GL_FindImage

	image->width = width;
	image->height = height;
	image->flags = flags;

	if ( compressed )
	{
		image->texnum = GL_UploadCompressed( pic, flags );
	}
	else
	{
		image->texnum = GL_Upload( pic, width, height, flags );
	}

	image->sl = 0;
	image->sh = 1;
	image->tl = 0;
	image->th = 1;

	return image;
}

// ensures that a DDS fits our requirements
static bool GL_CategorizeDDS( const char *pName, const byte *pBuffer, int bufLen )
{
	// first, check we're at least as long as the header
	if ( bufLen < sizeof( img::DDS_HEADER ) )
	{
		Com_Printf( "DDS file %s is smaller than 128 bytes!\n", pName );
		return false;
	}

	const img::DDS_HEADER *pHeader = (const img::DDS_HEADER *)pBuffer;

	// second, check to see if we're DX10, we only support DX10
	if ( !( pHeader->ddspf.flags & DDS_FOURCC ) ||
		 !( pHeader->ddspf.fourCC == MakeFourCC( 'D', 'X', '1', '0' ) ) )
	{
		Com_Printf( "DDS file %s must be DX10 format!\n", pName );
		return false;
	}

	// thirdly, make sure we're <larger> than the header plus dx10 ext header
	if ( bufLen <= sizeof( img::DDS_HEADER ) + sizeof( img::DDS_HEADER_DXT10 ) )
	{
		Com_Printf( "DDS file %s is smaller than 128 + 20 bytes!\n", pName );
		return false;
	}

	// we are okay!
	// TODO: account for formats we don't want in the DX10 ext header
	// TODO: test the ext header for Texture2D, general verification of 2D data

	return true;
}

//-------------------------------------------------------------------------------------------------
// Loads any of the supported image types into a cannonical 32 bit format.
//-------------------------------------------------------------------------------------------------
static bool GL_LoadImage( const char *pName, int &width, int &height, byte *&pPic )
{
	byte *pBuffer;
	fsSize_t nBufLen = FileSystem::LoadFile( pName, (void **)&pBuffer );
	if ( !pBuffer )
	{
		return false;
	}

	assert( nBufLen > 32 ); // Sanity check

	if ( img::TestPNG( pBuffer ) )
	{
		pPic = img::LoadPNG( pBuffer, width, height );

		FileSystem::FreeFile( pBuffer );
		return false;
	}
	else if ( img::TestDDS( pBuffer ) )
	{
		// if we're a dds, pPic becomes the file buffer

		if ( !GL_CategorizeDDS( pName, pBuffer, nBufLen ) ) {
			return false;
		}

		pPic = pBuffer;

		return true;
	}
	else
	{
		// There is no real test for TGA
		pPic = img::LoadTGA( pBuffer, nBufLen, width, height );

		FileSystem::FreeFile( pBuffer );
		return false;
	}

	// TODO: Unreachable? This function kinda sucks
	
	Com_Printf( "GL_LoadImage - %s is an unsupported image format!", pName );
	return false;
}

//-------------------------------------------------------------------------------------------------
// Finds or loads the given image
//
// THIS CHOKES ON TEXTURES WITH THE SAME NAME, STUDIOMODELS CAN HAVE IDENTICAL TEXTURE NAMES THAT ARE
// DIFFERENT!!! THIS IS BAD!
//-------------------------------------------------------------------------------------------------
static image_t *GL_FindImage( const char *name, imageFlags_t flags )
{
	int i;
	image_t *image;
	int width, height;

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
	// load the pic from disk
	//
	byte *pic = nullptr;

	bool compressed = GL_LoadImage( name, width, height, pic );
	if ( !pic ) {
		defaultMaterial->image->IncrementRefCount();
		return defaultMaterial->image;
	}

	image = GL_CreateImage( name, pic, width, height, flags, compressed );

	// HACK: this is weird
	if ( compressed ) {
		Mem_Free( pic );
	} else {
		FileSystem::FreeFile( pic );
	}

	++image->refcount;
	return image;
}

bool ParseMaterial( char *data, material_t *material )
{
	char tokenhack[MAX_TOKEN_CHARS];
	char *token = tokenhack;

	char baseTexture[MAX_TOKEN_CHARS]; // For storing off the basetexture for later
	baseTexture[0] = '\0';

	char specTexture[MAX_TOKEN_CHARS];
	specTexture[0] = '\0';

	char normTexture[MAX_TOKEN_CHARS];
	normTexture[0] = '\0';

	char emitTexture[MAX_TOKEN_CHARS];
	emitTexture[0] = '\0';

	imageFlags_t flags = 0;

	COM_Parse2( &data, &token, sizeof( tokenhack ) );
	if ( token[0] != '{' )
	{
		// Malformed
		Com_Printf( "Malformed material %s\n", material->name );
		return false;
	}

	COM_Parse2( &data, &token, sizeof( tokenhack ) );

	for ( ; token[0] != '}'; COM_Parse2( &data, &token, sizeof( tokenhack ) ) )
	{
		if ( Q_strcmp( token, "$basetexture" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			Q_strcpy_s( baseTexture, token );
			flags |= IF_SRGB;
			continue;
		}
		if ( Q_strcmp( token, "$spectexture" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			Q_strcpy_s( specTexture, token );
			continue;
		}
		if ( Q_strcmp( token, "$normtexture" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			Q_strcpy_s( normTexture, token );
			continue;
		}
		if ( Q_strcmp( token, "$emittexture" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			Q_strcpy_s( emitTexture, token );
			continue;
		}
		if ( Q_strcmp( token, "$nextframe" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			material->nextframe = GL_FindMaterial( token );
			if ( material->nextframe == defaultMaterial ) // SLARTHACK: THIS IS REALLY BAD!!! REFCOUNT!!!
			{
				material->nextframe = nullptr;
			}
			continue;
		}
		if ( Q_strcmp( token, "$nomips" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			if ( Q_atoi( token ) )
			{
				flags |= IF_NOMIPS;
			}
			continue;
		}
		if ( Q_strcmp( token, "$noaniso" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			if ( Q_atoi( token ) )
			{
				flags |= IF_NOANISO;
			}
			continue;
		}
		if ( Q_strcmp( token, "$nearestfilter" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			if ( Q_atoi( token ) )
			{
				flags |= IF_NEAREST;
			}
			continue;
		}
		if ( Q_strcmp( token, "$clamps" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			if ( Q_atoi( token ) )
			{
				flags |= IF_CLAMPS;
			}
			continue;
		}
		if ( Q_strcmp( token, "$clampt" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			if ( Q_atoi( token ) )
			{
				flags |= IF_CLAMPT;
			}
			continue;
		}
		if ( Q_strcmp( token, "$alpha" ) == 0 )
		{
			// stored as a float for user convenience (it's easier to understand 0.5 as half transparency than 128 is)
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			material->alpha = Clamp( static_cast<uint32>( Q_atod( token ) * 255.0 + 0.5 ), 0u, 255u );
			continue;
		}
	}

	if ( baseTexture[0] )
	{
		material->image = GL_FindImage( baseTexture, flags );
	}
	else
	{
		material->image = defaultMaterial->image;
		material->image->IncrementRefCount();
	}

	if ( specTexture[0] )
	{
		material->specImage = GL_FindImage( specTexture, flags );
	}
	else
	{
		material->specImage = blackMaterial->image;
		material->specImage->IncrementRefCount();
	}

	if ( normTexture[0] )
	{
		material->normImage = GL_FindImage( normTexture, flags );
	}
	else
	{
		material->normImage = flatNormalImage;
		material->normImage->IncrementRefCount();
	}

	if ( emitTexture[0] )
	{
		material->emitImage = GL_FindImage( emitTexture, flags );
	}
	else
	{
		material->emitImage = blackMaterial->image;
		material->emitImage->IncrementRefCount();
	}

	return true;
}

static material_t *GL_CreateMaterialFromData( const char *name, image_t *image, image_t *specImage, image_t *normImage, image_t *emitImage )
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
			Com_Errorf("MAX_GLMATERIALS" );
		++numglmaterials;
	}
	material = &glmaterials[i];
	memset( material, 0, sizeof( *material ) );
	
	Q_strcpy_s( material->name, name );

	// gotcha: increment refcount
	image->IncrementRefCount();
	material->image = image;
	specImage->IncrementRefCount();
	material->specImage = specImage;
	normImage->IncrementRefCount();
	material->normImage = normImage;
	emitImage->IncrementRefCount();
	material->emitImage = emitImage;

	material->alpha = 0xFF;

	material->registration_sequence = -1; // Data materials are always managed

	return material;
}

static material_t *GL_CreateMaterial( const char *name )
{
	assert( !strstr( name, "pcx" ) );

	// Horrible hack, clean this up
#if 0
	char newname[MAX_QPATH];
	assert( strlen( name ) < MAX_QPATH );
	Q_strcpy_s( newname, name );
	strcat( newname, ".mat" );
#endif

	char *pBuffer;
	fsSize_t nBufLen = FileSystem::LoadFile( name, (void **)&pBuffer, 1 );
	if ( !pBuffer )
	{
		return defaultMaterial;
	}

	// find a free material_t
	int i;
	material_t *material;
	for ( i = 0, material = glmaterials; i < numglmaterials; i++, material++ )
	{
		if ( material->image == nullptr )
			break;
	}
	if ( i == numglmaterials )
	{
		if ( numglmaterials == MAX_GLMATERIALS )
			Com_Error("MAX_GLMATERIALS" ); // This can probably just return defaultMaterial
		++numglmaterials;
	}
	material = &glmaterials[i];
	memset( material, 0, sizeof( *material ) );

	Q_strcpy_s( material->name, name );

	material->image = nullptr;
	material->specImage = nullptr;
	material->normImage = nullptr;
	material->emitImage = nullptr;

	material->alpha = 0xFF;

	if ( !ParseMaterial( pBuffer, material ) )
	{
		FileSystem::FreeFile( pBuffer );
		memset( material, 0, sizeof( *material ) );
		return defaultMaterial;
	}

	FileSystem::FreeFile( pBuffer );
	return material;
}

material_t *GL_FindMaterial( const char *name, bool managed /*= false*/ )
{
	int i;
	material_t *material;
	char newname[MAX_QPATH];

	if ( strstr( name, "players/" ) )
	{
		// No, no screw you
		return defaultMaterial;
	}

	if ( !strstr( name, MAT_EXT ) )
	{
		// This is a legacy path, we'll need to prepend materials/ and append with .mat
		strcpy( newname, "materials/" );
		Com_FileSetExtension( name, newname + sizeof( "materials/" ) - 1, MAT_EXT );
	}
	else
	{
		Q_strcpy_s( newname, name );
	}

	// look for it
	for ( i = 0, material = glmaterials; i < numglmaterials; ++i, ++material )
	{
		if ( Q_strcmp( newname, material->name ) == 0 )
		{
			material->registration_sequence = managed ? -1 : tr.registrationSequence;
			return material;
		}
	}

	// TODO: Managed materials get their registration sequence melted here

	material = GL_CreateMaterial( newname );
	material->registration_sequence = managed ? -1 : tr.registrationSequence;

	return material;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
material_t *R_RegisterSkin( const char *name )
{
	// This is only used by the terrible post-release multiplayer code
	return defaultMaterial;
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

		if ( image == defaultMaterial->image || image == mat_particletexture->image )
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
		if ( material->registration_sequence == tr.registrationSequence )
			continue;		// used this sequence
		if ( material->registration_sequence <= 0 )
			continue;		// free slot or managed

		Com_DPrintf( "Clearing %s\n", material->name );

		material->Delete();
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void GL_GetPalette (void)
{
	byte *pBuffer;
	fsSize_t nBufLen = FileSystem::LoadFile("textures/colormap.pcx", (void**)&pBuffer);
	if (!pBuffer)
	{
		Com_FatalError("Couldn't load textures/colormap.pcx\n");
	}

	if (!img::CreateColormapFromPCX(pBuffer, nBufLen, d_8to24table))
	{
		FileSystem::FreeFile(pBuffer);
		Com_FatalError("textures/colormap.pcx is not a valid PCX!\n");
	}

	FileSystem::FreeFile(pBuffer);
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
#if 0
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
#endif

#define	DEFAULT_SIZE	16

static void R_CreateIntrinsicImages()
{
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// black image

	memset( data, 0, sizeof( data ) );
	image_t *blackImage = GL_CreateImage( "***blackImage***", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, IF_NOMIPS | IF_NEAREST, false );

	// white image

	memset( data, 255, sizeof( data ) );
	image_t *whiteImage = GL_CreateImage( "***whiteImage***", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, IF_NOMIPS | IF_NEAREST, false );

	// flat normal map

	int x, y;

	for ( y = 0; y < DEFAULT_SIZE; ++y ) {
		for ( x = 0; x < DEFAULT_SIZE; ++x ) {
			data[y][x][0] = 128;
			data[y][x][1] = 128;
			data[y][x][2] = 255;
			data[y][x][3] = 255;
		}
	}
	flatNormalImage = GL_CreateImage( "***flatNormalImage***", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, 0, false );

	// black and white materials

	blackMaterial = GL_CreateMaterialFromData( "***blackMaterial***", blackImage, blackImage, flatNormalImage, blackImage );
	whiteMaterial = GL_CreateMaterialFromData( "***whiteMaterial***", whiteImage, blackImage, flatNormalImage, blackImage );

	// default texture

	// grey center
	for ( y = 0; y < DEFAULT_SIZE; ++y ) {
		for ( x = 0; x < DEFAULT_SIZE; ++x ) {
			data[y][x][0] = 32;
			data[y][x][1] = 32;
			data[y][x][2] = 32;
			data[y][x][3] = 255;
		}
	}
	// white border
	for ( x = 0; x < DEFAULT_SIZE; ++x ) {
		data[0][x][0] = 255;
		data[0][x][1] = 255;
		data[0][x][2] = 255;
		data[0][x][3] = 255;

		data[x][0][0] = 255;
		data[x][0][1] = 255;
		data[x][0][2] = 255;
		data[x][0][3] = 255;

		data[DEFAULT_SIZE-1][x][0] = 255;
		data[DEFAULT_SIZE-1][x][1] = 255;
		data[DEFAULT_SIZE-1][x][2] = 255;
		data[DEFAULT_SIZE-1][x][3] = 255;

		data[x][DEFAULT_SIZE-1][0] = 255;
		data[x][DEFAULT_SIZE-1][1] = 255;
		data[x][DEFAULT_SIZE-1][2] = 255;
		data[x][DEFAULT_SIZE-1][3] = 255;
	}
	image_t *defaultImage = GL_CreateImage( "***defaultImage***", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, IF_NONE, false );
	defaultMaterial = GL_CreateMaterialFromData( "***defaultMaterial***", defaultImage, whiteImage, flatNormalImage, blackImage );
}

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void GL_InitImages( void )
{
	GL_GetPalette();

	GL_BuildGammaTable( r_gamma->GetFloat(), 2 );

	R_CreateIntrinsicImages();

	g_imagesInitialised = true;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void GL_ShutdownImages( void )
{
	int i;
	material_t *material;

	if ( !g_imagesInitialised ) {
		return;
	}

	// Clear our generated materials
//	extern material_t *draw_chars;
//	draw_chars->Delete();
//	mat_particletexture->Delete();
//	defaultMaterial->Delete();

	// Clear materials, images are cleared automatically
	for ( i = 0, material = glmaterials; i < numglmaterials; ++i, ++material )
	{
		if ( material->registration_sequence == 0 )
			continue; // Free slot

		// Decrement refcount
		material->Delete();
	}

	// Images are dereferenced by the material when their refcount reaches 0
	// Go through every single image for security
#ifdef Q_DEBUG
	image_t *image;
	for ( i = 0, image = gltextures; i < MAX_GLTEXTURES; ++i, ++image )
	{
		assert( image->refcount == 0 && image->texnum == 0 );
	}
#endif
}

void *R_GetDefaultTexture()
{
	return (void *)(intptr_t)defaultMaterial->image->texnum;
}

/*
===============================================================================
	Framebuffers
===============================================================================
*/

#define MAX_FRAMEBUFFERS 16

struct frameBuffer_t
{
	GLuint obj, color, depthStencil;
	int width, height;
};

static frameBuffer_t s_frameBuffers[MAX_FRAMEBUFFERS];

int R_CreateFBO( int width, int height )
{
	// find a free framebuffer
	int i = 0;
	for ( ; i < MAX_FRAMEBUFFERS; ++i )
	{
		if ( s_frameBuffers[i].obj == 0 ) {
			break;
		}
	}
	if ( i == MAX_FRAMEBUFFERS ) {
		Com_FatalError( "Ran out of framebuffer slots, tell a programmer!\n" );
	}

	frameBuffer_t &fb = s_frameBuffers[i];

	fb.width = width;
	fb.height = height;

	glGenFramebuffers( 1, &fb.obj );
	glBindFramebuffer( GL_FRAMEBUFFER, fb.obj );

	glGenTextures( 1, &fb.color );
	GL_BindTexture( fb.color );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );
	GL_ApplyTextureParameters( IF_NOMIPS );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.color, 0 );

	glGenRenderbuffers( 1, &fb.depthStencil );
	glBindRenderbuffer( GL_RENDERBUFFER, fb.depthStencil );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height );
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb.depthStencil );

	if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
	{
		Com_FatalError( "Failed to create framebuffer\n" );
	}

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	return i;
}

void R_DestroyFBO( int fbo )
{
	assert( fbo >= 0 && fbo < MAX_FRAMEBUFFERS );
	frameBuffer_t &fb = s_frameBuffers[fbo];

	glDeleteRenderbuffers( 1, &fb.depthStencil );
	glDeleteTextures( 1, &fb.color );
	glDeleteFramebuffers( 1, &fb.obj );

	memset( &fb, 0, sizeof( fb ) );
}

void R_BindFBO( int fbo )
{
	assert( fbo >= 0 && fbo < MAX_FRAMEBUFFERS );
	const frameBuffer_t &fb = s_frameBuffers[fbo];

	glBindFramebuffer( GL_FRAMEBUFFER, fb.obj );

	glViewport( 0, 0, fb.width, fb.height );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void R_BindDefaultFBO()
{
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	int width, height;
	R_GetWindowDimensions( width, height );
	glViewport( 0, 0, width, height );
}

uint R_TexNumFBO( int fbo )
{
	assert( fbo >= 0 && fbo < MAX_FRAMEBUFFERS );
	const frameBuffer_t &fb = s_frameBuffers[fbo];

	return fb.color;
}

void R_DestroyAllFBOs()
{
	for ( int i = 0; i < MAX_FRAMEBUFFERS; ++i )
	{
		if ( s_frameBuffers[i].obj != 0 ) {
			R_DestroyFBO( i );
		}
	}
}
