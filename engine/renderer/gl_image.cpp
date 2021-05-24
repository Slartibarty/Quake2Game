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

//-------------------------------------------------------------------------------------------------
// This must NEVER fail
//
// Only mipmap select types
// We want to mipmap walls, skins and sprites, although never
// 2D pics or skies
//-------------------------------------------------------------------------------------------------
static GLuint GL_Upload32( const byte *pData, int nWidth, int nHeight, imageFlags_t flags )
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
			glGenerateMipmap( GL_TEXTURE_2D );
		}
#if 0
		else
		{
			// This sucks even more
			int nOutWidth = nWidth / 2;
			int nOutHeight = nHeight / 2;
			int nLevel = 1;

			byte *pNewData = (byte *)Mem_Alloc( ( nOutWidth * nOutHeight ) * 4 );

			while ( nOutWidth > 1 && nOutHeight > 1 )
			{
				stbir_resize_uint8( pData, nWidth, nHeight, 0, pNewData, nOutWidth, nOutHeight, 0, 4 );
				glTexImage2D( GL_TEXTURE_2D, nLevel, GL_RGBA8, nOutWidth, nOutHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pNewData );

				++nLevel;
				nOutWidth /= 2;
				nOutHeight /= 2;
			}

			Mem_Free( pNewData );
		}
#endif

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
static image_t *GL_CreateImage(const char *name, const byte *pic, int width, int height, imageFlags_t flags)
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
			Com_Errorf("MAX_GLTEXTURES");
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
static byte *GL_LoadImage( const char *pName, int &width, int &height )
{
	byte *pBuffer;
	int nBufLen;

	nBufLen = FS_LoadFile( pName, (void **)&pBuffer );
	if ( !pBuffer )
	{
		return nullptr;
	}

	assert( nBufLen > 32 ); // Sanity check

	byte *pPic = nullptr;

	if ( img::TestPNG( pBuffer ) )
	{
		pPic = img::LoadPNG( pBuffer, width, height );
	}
	else
	{
		// There is no real test for TGA
		pPic = img::LoadTGA( pBuffer, nBufLen, width, height );
	}
	
	if ( !pPic )
	{
		Com_Printf( "GL_LoadImage - %s is an unsupported image format!", pName );
	}

	FS_FreeFile( pBuffer );

	if ( !pPic )
	{
		return nullptr;
	}

	return pPic;
}

//-------------------------------------------------------------------------------------------------
// Finds or loads the given image
//
// THIS CHOKES ON TEXTURES WITH THE SAME NAME, STUDIOMODELS CAN HAVE IDENTICAL TEXTURE NAMES THAT ARE
// DIFFERENT!!! THIS IS BAD!
//-------------------------------------------------------------------------------------------------
static image_t *GL_FindImage (const char *name, imageFlags_t flags)
{
	int i;
	image_t *image;
	int width, height;
	byte *pic;

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

#if 0
	//
	// load the pic from an 8-bit buffer (studiomodels)
	//
	if ( pic )
	{
		int outsize = width * height * 4;
		byte *pic32 = (byte *)Mem_Alloc( outsize );
		rgb_t *palette = (rgb_t *)( pic + width * height );

		for ( i = 0, j = 0; i < outsize; i += 4, ++j )
		{
			pic32[i+0] = palette[pic[j]].r;
			pic32[i+1] = palette[pic[j]].g;
			pic32[i+2] = palette[pic[j]].b;
			pic32[i+3] = 255; // SlartTodo: Studiomodels can have alphatest transparency
		}
		
		image = GL_CreateImage( name, pic32, width, height, flags );

		Mem_Free( pic32 );

		++image->refcount;
		return image;
	}
#endif

	//
	// load the pic from disk
	//
	pic = GL_LoadImage( name, width, height );
	if ( !pic ) {
		defaultMaterial->image->IncrementRefCount();
		return defaultMaterial->image;
	}

	image = GL_CreateImage( name, pic, width, height, flags );

	Mem_Free( pic );

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
		if ( Q_strcmp( token, "$alpha" ) == 0 )
		{
			// stored as a float for user convenience (it's easier to understand 0.5 as half transparency than 128 is)
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			material->alpha = Clamp( static_cast<uint32>( atof( token ) * 255.0 + 0.5 ), 0u, 255u );
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
	char *pBuffer;
	int nBufLen;
	int i;
	material_t *material;

	assert( !strstr( name, "pcx" ) );

	// Horrible hack, clean this up
#if 0
	char newname[MAX_QPATH];
	assert( strlen( name ) < MAX_QPATH );
	Q_strcpy_s( newname, name );
	strcat( newname, ".mat" );
#endif

	nBufLen = FS_LoadFile( name, (void **)&pBuffer, 1 );
	if ( !pBuffer )
	{
		return defaultMaterial;
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
		FS_FreeFile( pBuffer );
		memset( material, 0, sizeof( *material ) );
		return defaultMaterial;
	}

	FS_FreeFile( pBuffer );
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
	byte	*pBuffer;
	int		nBufLen;

	nBufLen = FS_LoadFile("textures/colormap.pcx", (void**)&pBuffer);
	if (!pBuffer)
	{
		Com_FatalError("Couldn't load textures/colormap.pcx");
	}

	if (!img::CreateColormapFromPCX(pBuffer, nBufLen, d_8to24table))
	{
		FS_FreeFile(pBuffer);
		Com_FatalError("textures/colormap.pcx is not a valid PCX!");
	}

	FS_FreeFile(pBuffer);
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
	image_t *blackImage = GL_CreateImage( "***blackImage***", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, IF_NOMIPS | IF_NEAREST );

	// white image

	memset( data, 255, sizeof( data ) );
	image_t *whiteImage = GL_CreateImage( "***whiteImage***", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, IF_NOMIPS | IF_NEAREST );

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
	flatNormalImage = GL_CreateImage( "***flatNormalImage***", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, 0 );

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
	image_t *defaultImage = GL_CreateImage("***defaultImage***", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, IF_NONE);
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
