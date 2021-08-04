
#include "gl_local.h"

#include "../shared/imgtools.h"
#include "../../core/threading.h"

/*
===================================================================================================

	Screenshots

===================================================================================================
*/

static constexpr size_t MaxScreenshots = 999;

struct threadData_t
{
	char	filename[MAX_QPATH];
	byte *	pixBuffer;
	int		width, height;
};

static uint32 PNG_ThreadProc( void *params )
{
	threadData_t *threadData = (threadData_t *)params;

	fsHandle_t handle = FileSystem::OpenFileWrite( threadData->filename );
	if ( !handle ) {
		Mem_Free( threadData->pixBuffer );
		Mem_Free( threadData );
		return 1;
	}

	img::VerticalFlip( threadData->pixBuffer, vid.width, vid.height, 3 );
	img::WritePNG( vid.width, vid.height, false, threadData->pixBuffer, handle );

	FileSystem::CloseFile( handle );
	Mem_Free( threadData->pixBuffer );
	Mem_Free( threadData );

	return 0;
}

/*
========================
GL_Screenshot_Internal
========================
*/
static void GL_Screenshot_Internal( bool png )
{	
	// create the screenshots directory if it doesn't exist
	char checkname[MAX_QPATH];
	strcpy( checkname, "screenshots" );
	FileSystem::CreatePath( checkname );

	// find a file name to save it to
	char picname[16];
	Q_sprintf_s( picname, "quake00.%s", png ? "png" : "tga" );

	size_t i;

	for ( i = 0; i <= MaxScreenshots; ++i )
	{
		picname[5] = i / 10 + '0';
		picname[6] = i % 10 + '0';
		Q_sprintf_s( checkname, "screenshots/%s", picname );
		if ( !FileSystem::FileExists( checkname, FS_WRITEDIR ) ) {
			// file doesn't exist
			break;
		}
	}
	if ( i == MaxScreenshots + 1 )
	{
		Com_Print( "GL_Screenshot: Couldn't create a file\n" );
		return;
	}

	size_t c = (size_t)vid.width * (size_t)vid.height * 3;
	size_t addsize = 0;
	if ( !png )
	{
		addsize = 18; // TGA Header
		c += addsize;
	}
	byte *pixbuffer = (byte *)Mem_Alloc( c );

	glReadPixels( 0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, pixbuffer + addsize );

	if ( png )
	{
		Com_Printf( "Writing %s\n", picname );

		threadData_t *threadData = (threadData_t *)Mem_Alloc( sizeof( threadData_t ) );
		strcpy( threadData->filename, checkname );
		threadData->pixBuffer = pixbuffer;
		threadData->width = vid.width;
		threadData->height = vid.height;

		// fire and forget
		threadHandle_t thread = Sys_CreateThread( PNG_ThreadProc, threadData, THREAD_NORMAL, PLATTEXT( "PNG Screenshot Thread" ), CORE_ANY );
		Sys_DestroyThread( thread );
	}
	else
	{
		fsHandle_t handle = FileSystem::OpenFileWrite( checkname );
		assert( handle );

		memset( pixbuffer, 0, 18 );
		pixbuffer[2] = 2; // uncompressed type
		pixbuffer[12] = vid.width & 255;
		pixbuffer[13] = vid.width >> 8;
		pixbuffer[14] = vid.height & 255;
		pixbuffer[15] = vid.height >> 8;
		pixbuffer[16] = 24; // pixel size

		byte temp;
		// swap rgb to bgr
		for ( i = 18; i < c; i += 3 )
		{
			temp = pixbuffer[i];
			pixbuffer[i] = pixbuffer[i + 2];
			pixbuffer[i + 2] = temp;
		}

		FileSystem::WriteFile( pixbuffer, c, handle );

		FileSystem::CloseFile( handle );
		Mem_Free( pixbuffer );

		Com_Printf( "Wrote %s\n", picname );
	}
}

void GL_Screenshot_PNG_f()
{
	GL_Screenshot_Internal( true );
}

void GL_Screenshot_TGA_f()
{
	GL_Screenshot_Internal( false );
}

/*
===================================================================================================

	Console commands

===================================================================================================
*/

struct rgb_t
{
	byte r, g, b;
};

/*
========================
R_ExtractWad_f

Extract a WAD file to 24-bit TGA files
========================
*/
void R_ExtractWad_f()
{
#if 0
	if ( CmdSystem::GetArgc() < 3 )
	{
		Com_Printf( "Usage: <file.wad> <palette.lmp>\n" );
		return;
	}

	char wadbase[MAX_QPATH];
	COM_FileBase( CmdSystem::GetArgv( 1 ), wadbase );

	// Create the output folder if it doesn't exist
	Sys_CreateDirectory( va( "%s/textures/%s", FS_Gamedir(), wadbase ) );

	FILE *wadhandle;

	wadhandle = fopen( va( "%s/%s", FS_Gamedir(), CmdSystem::GetArgv( 2 ) ), "rb" );
	if ( !wadhandle )
	{
		Com_Printf( "Couldn't open %s\n", CmdSystem::GetArgv( 2 ) );
		return;
	}

	byte palette[256 * 3];

	if ( fread( palette, 3, 256, wadhandle ) != 256 )
	{
		fclose( wadhandle );
		Com_Printf( "Malformed palette lump\n" );
		return;
	}

	fclose( wadhandle );

	wadhandle = fopen( va( "%s/%s", FS_Gamedir(), CmdSystem::GetArgv( 1 ) ), "rb" );
	if ( !wadhandle )
	{
		Com_Printf( "Couldn't open %s\n", CmdSystem::GetArgv( 1 ) );
		return;
	}

	wad2::wadinfo_t wadheader;

	if ( fread( &wadheader, sizeof( wadheader ), 1, wadhandle ) != 1 || wadheader.identification != wad2::IDWADHEADER )
	{
		fclose( wadhandle );
		Com_Printf( "Malformed wadfile\n" );
		return;
	}

	Com_Printf( "Wad lumps: %d\n", wadheader.numlumps );

	fseek( wadhandle, wadheader.infotableofs, SEEK_SET );

	wad2::lumpinfo_t *wadlumps = (wad2::lumpinfo_t *)Mem_Alloc( sizeof( wad2::lumpinfo_t ) * wadheader.numlumps );

	if ( fread( wadlumps, sizeof( wad2::lumpinfo_t ), wadheader.numlumps, wadhandle ) != wadheader.numlumps )
	{
		Mem_Free( wadlumps );
		fclose( wadhandle );
		Com_Printf( "Malformed wadfile\n" );
		return;
	}

	for ( int32 i = 0; i < wadheader.numlumps; ++i )
	{
		assert( wadlumps[i].compression == wad2::CMP_NONE );

		Com_Printf( "  lump %d name: %s\n", i, wadlumps[i].name );

		if ( wadlumps[i].type != wad2::TYP_MIPTEX && wadlumps[i].type != wad2::TYP_SOUND ) {
			continue;
		}

		wad2::miptex_t miptex;

		fseek( wadhandle, wadlumps[i].filepos, SEEK_SET );
		fread( &miptex, sizeof( miptex ), 1, wadhandle );

		assert( miptex.nOffsets[0] == 40 );

		int32 c;
		byte *pic8, *pic24;

		c = miptex.width * miptex.height;

		pic8 = (byte *)Mem_Alloc( c );

		fread( pic8, 1, c, wadhandle );

		pic24 = (byte *)Mem_Alloc( c * 3 );

		for ( int32 pixel = 0, pixel2 = 0; pixel < ( c * 3 ); pixel += 3, pixel2 += 3 )
		{
			assert( pixel2 < c );
			pic24[pixel + 0] = palette[pic8[pixel + 0]];
			pic24[pixel + 1] = palette[pic8[pixel + 1]];
			pic24[pixel + 2] = palette[pic8[pixel + 2]];
		}

		Mem_Free( pic8 );

		char outname[MAX_QPATH];
		Q_sprintf_s( outname, "%s/textures/%s/%s.png", FS_Gamedir(), wadbase, wadlumps[i].name );

		char *asterisk;
		while ( ( asterisk = strchr( outname, '*' ) ) )
		{
			// Half-Lifeify the texture name
			*asterisk = '!';
		}

		FILE *outHandle = fopen( outname, "wb" );
		if ( outHandle )
		{
			if ( !img::WritePNG( miptex.width, miptex.height, false, pic24, outHandle ) )
			{
				Com_Printf( "Failed to write %s\n", outname );
			}
			fclose( outHandle );
		}

		Mem_Free( pic24 );
	}

	Mem_Free( wadlumps );
	fclose( wadhandle );
#endif
}

/*
========================
R_LoadWAL

Loads a WAL (Remember those?)
========================
*/
static byte *R_LoadWAL(const byte *pBuffer, int nBufLen, int &width, int &height)
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
	uint *pPic32 = (uint *)Mem_Alloc(c * 4);

	for (int i = 0; i < c; ++i)
	{
		pPic32[i] = d_8to24table[pPic8[i]];
	}

	return (byte *)pPic32;
}

/*
========================
R_UpgradeWals_f

Upgrade all WALs in a specified folder to TGAs
========================
*/
void R_UpgradeWals_f()
{
#if 0
	if ( CmdSystem::GetArgc() < 2 )
	{
		Com_Printf( "Usage: <folder>\n" );
		return;
	}

	char		folder[MAX_OSPATH];
	const char	*str;

	Q_sprintf_s( folder, "%s/%s/*", FS_Gamedir(), CmdSystem::GetArgv( 1 ) );

	str = Sys_FindFirst( folder, 0, 0 );
	if ( !str ) {
		Com_Printf( "No files found in folder\n" );
		Sys_FindClose();
		return;
	}

	for ( ; str; str = Sys_FindNext( 0, 0 ) )
	{
		if ( strstr( str, ".wal" ) == NULL )
			continue;
		Com_Printf( "Upgrading %s\n", str );

		FILE *handle = fopen( str, "rb" );

		fseek( handle, 0, SEEK_END );
		int length = ftell( handle );
		fseek( handle, 0, SEEK_SET );

		byte *file = (byte *)Mem_Alloc( length );

		fread( file, 1, length, handle );

		fclose( handle );

		int width, height;
		byte *data = R_LoadWAL( file, length, width, height );
		Mem_Free( file );

		char tempname[MAX_QPATH];
		Q_strcpy_s( tempname, str );
		strcpy( strstr( tempname, ".wal" ), ".png" );

		handle = fopen( tempname, "wb" );
		assert( handle );
		img::WritePNG( width, height, true, data, handle );
		fclose( handle );

		Mem_Free( data );
	}

	Sys_FindClose();
#endif
}

/*
===================================================================================================

	OpenGL state helpers

===================================================================================================
*/

void GL_EnableMultitexture(bool enable)
{
	if (!GLEW_ARB_multitexture || !r_ext_multitexture->GetBool())
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
	if (!GLEW_ARB_multitexture || !r_ext_multitexture->GetBool())
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

	if (tmu == glState.currenttmu)
	{
		return;
	}

	glState.currenttmu = tmu;

	glActiveTextureARB(texture);
	glClientActiveTextureARB(texture);
}

void GL_TexEnv(GLint mode)
{
#if 0
	static GLint lastmodes[2] = { -1, -1 };

	if (mode != lastmodes[glState.currenttmu])
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
		lastmodes[glState.currenttmu] = mode;
	}
#endif
}

void GL_Bind( GLuint texnum )
{
	// Are we already bound?
	if ( glState.currenttextures[glState.currenttmu] == texnum )
		return;

	glState.currenttextures[glState.currenttmu] = texnum;
	glBindTexture( GL_TEXTURE_2D, texnum );
}

void GL_MBind(GLenum target, GLuint texnum)
{
	GL_SelectTexture(target);
	if (target == GL_TEXTURE0)
	{
		if (glState.currenttextures[0] == texnum)
			return;
	}
	else
	{
		if (glState.currenttextures[1] == texnum)
			return;
	}
	GL_Bind(texnum);
}

/*
========================
GL_CheckErrors
========================
*/
void GL_CheckErrors()
{
	const char *msg;
	GLenum err;

	while ((err = glGetError()) != GL_NO_ERROR)
	{
		switch ( err )
		{
		case GL_INVALID_ENUM:
			msg = "GL_INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			msg = "GL_INVALID_VALUE";
			break;
		case GL_INVALID_OPERATION:
			msg = "GL_INVALID_OPERATION";
			break;
		case GL_STACK_OVERFLOW:
			msg = "GL_STACK_OVERFLOW";
			break;
		case GL_STACK_UNDERFLOW:
			msg = "GL_STACK_UNDERFLOW";
			break;
		case GL_OUT_OF_MEMORY:
			msg = "GL_OUT_OF_MEMORY";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			msg = "GL_OUT_OF_MEMORY";
			break;
		case GL_CONTEXT_LOST:						// OpenGL 4.5 or ARB_KHR_robustness
			msg = "GL_OUT_OF_MEMORY";
			break;
		default:
			msg = "UNKNOWN ERROR!";
			break;
		}

		Com_Printf( "GL_CheckErrors: 0x%x - %s\n", err, msg );
	}
}
