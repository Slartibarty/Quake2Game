//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#include "gl_local.h"
#include "../shared/imageloaders.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_ONLY_TGA
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
			ri.Con_Printf(PRINT_ALL, "Bad pcx file (%i x %i) (%i x %i)\n", xmax + 1, ymax + 1, pcx->xmax, pcx->ymax);
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
			ri.Con_Printf(PRINT_ALL, "Bad pcx file (%i x %i) (%i x %i)\n", xmax + 1, ymax + 1, pcx->xmax, pcx->ymax);
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
			ri.Con_Printf(PRINT_DEVELOPER, "PCX file was malformed");
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

struct glmode_t
{
	const char *name;
	GLint minimize, maximize;
};

static glmode_t modes[]
{
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

static constexpr int NUM_GL_MODES = countof(modes);

static GLint	gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
static GLint	gl_filter_max = GL_LINEAR;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void GL_TextureMode(char *string)
{
	int		i;
	image_t *glt;

	for (i = 0; i < NUM_GL_MODES; i++)
	{
		if (!Q_stricmp(modes[i].name, string))
			break;
	}

	if (i == NUM_GL_MODES)
	{
		ri.Con_Printf(PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i = 0, glt = gltextures; i < numgltextures; i++, glt++)
	{
		if (glt->type != it_pic && glt->type != it_sky)
		{
			GL_Bind(glt->texnum);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

//-------------------------------------------------------------------------------------------------

image_t		gltextures[MAX_GLTEXTURES];
int			numgltextures;

byte		g_gammatable[256];

unsigned	d_8to24table[256];

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void GL_ImageList_f (void)
{
	int		i;
	image_t	*image;
	int		texels;

	ri.Con_Printf (PRINT_ALL, "------------------\n");
	texels = 0;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->texnum <= 0)
			continue;
		texels += image->width *image->height;
		switch (image->type)
		{
		case it_skin:
			ri.Con_Printf (PRINT_ALL, "M");
			break;
		case it_sprite:
			ri.Con_Printf (PRINT_ALL, "S");
			break;
		case it_wall:
			ri.Con_Printf (PRINT_ALL, "W");
			break;
		case it_pic:
			ri.Con_Printf (PRINT_ALL, "P");
			break;
		default:
			ri.Con_Printf (PRINT_ALL, " ");
			break;
		}

		ri.Con_Printf (PRINT_ALL,  " %3i %3i RGB: %s\n",
			image->width, image->height, image->name);
	}
	ri.Con_Printf (PRINT_ALL, "Total texel count (not counting mipmaps): %i\n", texels);
}

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// This must NEVER fail
//
// Only mipmap select types
// We want to mipmap walls, skins and sprites, although never
// 2D pics or skies
//-------------------------------------------------------------------------------------------------
static GLuint GL_Upload32 (const byte *pData, int nWidth, int nHeight, imagetype_t eType)
{
	// Lightscale everything!
//	GL_LightScaleTexture((byte*)data, width, height);

	GLuint id;

	// Generate and bind the texture
	glGenTextures(1, &id);
	GL_Bind(id);

	// Upload it
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pData);

	// Pics and skies should never be mipmapped
	if (eType != it_pic && eType != it_sky)
	{
		if (GLEW_ARB_framebuffer_object)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			int nOutWidth = nWidth / 2;
			int nOutHeight = nHeight / 2;
			int nLevel = 1;

			byte* pNewData = (byte*)malloc((nOutWidth * nOutHeight) * 4);

			while (nOutWidth > 1 && nOutHeight > 1)
			{
				stbir_resize_uint8(pData, nWidth, nHeight, 0, pNewData, nOutWidth, nOutHeight, 0, 4);
				glTexImage2D(GL_TEXTURE_2D, nLevel, GL_RGBA8, nOutWidth, nOutHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pNewData);

				++nLevel;
				nOutWidth /= 2;
				nOutHeight /= 2;
			}

			free(pNewData);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		// Only mipmappable textures should be anisotropic
		// It's safe to do this because we clamp it in GL_TextureAnisoMode in GL_SetDefaultState
		// Sprites should never be anisotropically filtered because they always face the player
		if (GLEW_ARB_texture_filter_anisotropic && eType != it_sprite) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 16.0f);
		}
	}
	else if (eType == it_pic)
	{
		// NEVER filter pics, EVER, it looks like ass
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}

	// Clamp everything that isn't lit
	if (GLEW_EXT_texture_edge_clamp && eType != it_wall && eType != it_skin)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE_EXT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE_EXT);
	}

	return id;
}

//-------------------------------------------------------------------------------------------------
// Loads any of the supported image types into a cannonical 32 bit format.
//-------------------------------------------------------------------------------------------------
static byte *GL_LoadImage(const char *pName, int &width, int &height)
{
	byte		*pBuffer;
	int			nBufLen;

	nBufLen = ri.FS_LoadFile(pName, (void **)&pBuffer);
	if (!pBuffer)
	{
		return nullptr;
	}

	byte *pPic;

	const char *pExt = strrchr(pName, '.') + 1;

	if (Q_stricmp(pExt, "tga") == 0)
	{
		// We use STB for TGA
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
		ri.FS_FreeFile(pBuffer);
		ri.Con_Printf(PRINT_ALL, "GL_LoadImage - %s is an unsupported image format!", pName);
		return nullptr;
	}

	if (!pPic)
	{
		return nullptr;
	}

	ri.FS_FreeFile(pBuffer);
	return pPic;
}

//-------------------------------------------------------------------------------------------------
// This is the only way any image_t are created
//-------------------------------------------------------------------------------------------------
static image_t *GL_CreateImage(const char *name, const byte *pic, int width, int height, imagetype_t type)
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
			ri.Sys_Error(ERR_DROP, "MAX_GLTEXTURES");
		numgltextures++;
	}
	image = &gltextures[i];

	Q_strcpy_s(image->name, name);
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;

	// Upload it!
	// Pics and skies are never mipmapped (they don't need em)
	image->texnum = GL_Upload32(pic, width, height, type);

	image->sl = 0;
	image->sh = 1;
	image->tl = 0;
	image->th = 1;

	return image;
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
image_t	*GL_FindImage (const char *name, imagetype_t type, byte *pic, int width, int height)
{
	assert( name && name[0] );

	// look for it
	image_t *image = gltextures;
	image_t *imagemax = image + numgltextures;
	for ( ; image <= imagemax; ++image )
	{
		if ( strcmp( name, image->name ) == 0 )
		{
			image->registration_sequence = registration_sequence;
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

		for ( int i = 0, j = 0; i < outsize; i += 4, ++j )
		{
			pic32[i+0] = palette[pic[j]].r;
			pic32[i+1] = palette[pic[j]].g;
			pic32[i+2] = palette[pic[j]].b;
			pic32[i+3] = 255; // SlartTodo: Studiomodels can have alphatest transparency
		}

	//	stbi_write_bmp( name, width, height, 4, pic32 );
		
		image = GL_CreateImage( name, pic32, width, height, type );

		free( pic32 );

		return image;
	}

	//
	// load the pic from disk
	//
	pic = GL_LoadImage( name, width, height );
	if ( !pic ) {
		return r_notexture;
	}

	image = GL_CreateImage( name, pic, width, height, type );

	free( pic );

	return image;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
image_t *R_RegisterSkin (const char *name)
{
	return GL_FindImage (name, it_skin);
}

//-------------------------------------------------------------------------------------------------
// Any image that was not touched on this registration sequence
// will be freed.
//-------------------------------------------------------------------------------------------------
void GL_FreeUnusedImages (void)
{
	// never free r_notexture or particle texture
	r_notexture->registration_sequence = registration_sequence;
	r_particletexture->registration_sequence = registration_sequence;

	int		i;
	image_t	*image;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if (image->type == it_pic)
			continue;		// don't free pics
		// free it
		glDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void GL_GetPalette (void)
{
	byte	*pBuffer;
	int		nBufLen;

	nBufLen = ri.FS_LoadFile("pics/colormap.pcx", (void**)&pBuffer);
	if (!pBuffer)
	{
		ri.Sys_Error(ERR_FATAL, "Couldn't load pics/colormap.pcx");
	}

	if (!ImageLoaders::CreateColormapFromPCX(pBuffer, nBufLen, d_8to24table))
	{
		ri.FS_FreeFile(pBuffer);
		ri.Sys_Error(ERR_FATAL, "pics/colormap.pcx is not a valid PCX!");
	}

	ri.FS_FreeFile(pBuffer);
}

//-------------------------------------------------------------------------------------------------
// Builds the gamma table
//-------------------------------------------------------------------------------------------------
static void GL_BuildGammaTable( float gamma, int overbright )
{
#if 1
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
#else
	int i, inf;

	int shift = 2;

	for ( i = 0; i < 256; i++ ) {
		if ( gamma == 1 ) {
			inf = i;
		}
		else {
			inf = static_cast<int>( 255 * pow( i / 255.0f, 1.0f / gamma ) + 0.5f );
		}
		inf <<= shift;
		if ( inf < 0 ) {
			inf = 0;
		}
		if ( inf > 255 ) {
			inf = 255;
		}
		g_gammatable[i] = inf;
	}
#endif
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
	{1,0,1,0,1,0,1,0},
	{0,1,0,1,0,1,0,1},
	{1,0,1,0,1,0,1,0},
	{0,1,0,1,0,1,0,1},
	{1,0,1,0,1,0,1,0},
	{0,1,0,1,0,1,0,1},
	{1,0,1,0,1,0,1,0},
	{0,1,0,1,0,1,0,1}
};

image_t *r_notexture;			// use for bad textures
image_t *r_particletexture;		// little dot for particles

static void R_InitParticleTexture(void)
{
	int		x, y;
	byte	data[8][8][4];

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
	r_particletexture = GL_CreateImage("***particle***", (byte *)data, 8, 8, it_sprite);

	//
	// also use this for bad textures, but without alpha
	//
	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			data[y][x][0] = missingtexture[x][y] * 255;
			data[y][x][1] = 0;
			data[y][x][2] = missingtexture[x][y] * 255;;
			data[y][x][3] = 255;
		}
	}
	r_notexture = GL_CreateImage("***r_notexture***", (byte *)data, 8, 8, it_pic);
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
	int		i;
	image_t	*image;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->registration_sequence == 0)
			continue;		// free image_t slot
		// free it
		glDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
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
