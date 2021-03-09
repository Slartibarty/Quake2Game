/*
=============================================================

Misc functions that don't fit anywhere else

=============================================================
*/

#include "gl_local.h"
#include "../shared/imageloaders.h"

#include "png.h"

//-------------------------------------------------------------------------------------------------
// Captures a 24-bit PNG screenshot
//-------------------------------------------------------------------------------------------------
static void GL_ScreenShot_Internal( bool png )
{
	size_t i;
	char checkname[MAX_OSPATH];
	char picname[64];
	FILE *handle;
	byte *pixbuffer;
	
// create the screenshots directory if it doesn't exist
	Q_sprintf_s( checkname, "%s/screenshots", FS_Gamedir() );
	Sys_CreateDirectory( checkname );

// find a file name to save it to 
	Q_sprintf_s( picname, "quake00.%s", png ? "png" : "tga" );

	for ( i = 0; i <= 99; ++i )
	{
		picname[5] = i / 10 + '0';
		picname[6] = i % 10 + '0';
		Q_sprintf_s( checkname, "%s/screenshots/%s", FS_Gamedir(), picname );
		handle = fopen( checkname, "rb" );
		if ( !handle )
			break;	// file doesn't exist
		fclose( handle );
	}
	if ( i == 100 )
	{
		Com_Printf( "GL_ScreenShot_f: Couldn't create a file\n" );
		return;
	}

	size_t c = (size_t)vid.width * (size_t)vid.height * 3;
	size_t addsize = 0;
	if ( !png )
	{
		addsize = 18; // TGA Header
		c += addsize;
	}
	pixbuffer = (byte *)Z_Malloc( c );

	glReadPixels( 0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, pixbuffer + addsize );

	handle = fopen( checkname, "wb" );
	assert( handle );

	if ( png )
	{
		img::VerticalFlip( pixbuffer, vid.width, vid.height, 3 );
		img::WritePNG( vid.width, vid.height, false, pixbuffer, handle );
	}
	else
	{
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

		fwrite( pixbuffer, 1, c, handle );
	}

	fclose( handle );

	Z_Free( pixbuffer );

	Com_Printf( "Wrote %s\n", picname );
}

void GL_ScreenShot_PNG_f()
{
	GL_ScreenShot_Internal( true );
}

void GL_ScreenShot_TGA_f()
{
	GL_ScreenShot_Internal( false );
}

//-------------------------------------------------------------------------------------------------
// Prints some OpenGL strings
//-------------------------------------------------------------------------------------------------
void GL_Strings_f()
{
	Com_Printf( "GL_VENDOR: %s\n", glGetString( GL_VENDOR ) );
	Com_Printf( "GL_RENDERER: %s\n", glGetString( GL_RENDERER ) );
	Com_Printf( "GL_VERSION: %s\n", glGetString( GL_VERSION ) );
	Com_Printf( "GL_SHADING_LANGUAGE_VERSION: %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ) );
//	Com_Printf( "GL_EXTENSIONS: %s\n", glGetString( GL_EXTENSIONS ) );
}

//-------------------------------------------------------------------------------------------------
// Extract a WAD file to 24-bit TGA files
//-------------------------------------------------------------------------------------------------
void GL_ExtractWad_f()
{
	if ( Cmd_Argc() < 3 )
	{
		Com_Printf( "Usage: <file.wad> <palette.lmp>\n" );
		return;
	}

	char wadbase[MAX_QPATH];
	COM_FileBase( Cmd_Argv( 1 ), wadbase );

	// Create the output folder if it doesn't exist
	Sys_CreateDirectory( va( "%s/textures/%s", FS_Gamedir(), wadbase ) );

	FILE *wadhandle;

	wadhandle = fopen( va( "%s/%s", FS_Gamedir(), Cmd_Argv( 2 ) ), "rb" );
	if ( !wadhandle )
	{
		Com_Printf( "Couldn't open %s\n", Cmd_Argv( 2 ) );
		return;
	}

	byte palette[256 * 3];

	if ( fread( palette, 3, 256, wadhandle ) != 256 )
	{
		fclose( wadhandle );
		Com_Printf( "Malformed palette lump\n" );
	}

	fclose( wadhandle );

	wadhandle = fopen( va( "%s/%s", FS_Gamedir(), Cmd_Argv( 1 ) ), "rb" );
	if ( !wadhandle )
	{
		Com_Printf( "Couldn't open %s\n", Cmd_Argv( 1 ) );
		return;
	}

	wad2::wadinfo_t wadheader;

	if ( fread( &wadheader, sizeof( wadheader ), 1, wadhandle ) != 1 || wadheader.identification != wad2::IDWADHEADER )
	{
		fclose( wadhandle );
		Com_Printf( "Malformed wadfile\n" );
		return;
	}

	fseek( wadhandle, wadheader.infotableofs, SEEK_SET );

	wad2::lumpinfo_t *wadlumps = (wad2::lumpinfo_t *)Z_Malloc( sizeof( wad2::lumpinfo_t ) * wadheader.numlumps );

	if ( fread( wadlumps, sizeof( wad2::lumpinfo_t ), wadheader.numlumps, wadhandle ) != wadheader.numlumps )
	{
		Z_Free( wadlumps );
		fclose( wadhandle );
		Com_Printf( "Malformed wadfile\n" );
		return;
	}

	for ( int32 i = 0; i < wadheader.numlumps; ++i )
	{
		assert( wadlumps[i].compression == wad2::CMP_NONE );

		if ( wadlumps[i].type != wad2::TYP_MIPTEX )
			continue;

		wad2::miptex_t miptex;

		fseek( wadhandle, wadlumps[i].filepos, SEEK_SET );
		fread( &miptex, sizeof( miptex ), 1, wadhandle );

		int32 c;
		byte *pic8, *pic32;

		c = miptex.width * miptex.height;

		pic8 = (byte *)Z_Malloc( c );

		fread( pic8, 1, c, wadhandle );

		pic32 = (byte *)Z_Malloc( c * 3 );

		for ( int32 pixel = 0; pixel < ( c * 3 ); pixel += 3 )
		{
			pic32[pixel + 0] = palette[pic8[pixel + 0]];
			pic32[pixel + 1] = palette[pic8[pixel + 1]];
			pic32[pixel + 2] = palette[pic8[pixel + 2]];
		}

		Z_Free( pic8 );

		char outname[MAX_QPATH];
		Q_sprintf_s( outname, "%s/textures/%s/%s.tga", FS_Gamedir(), wadbase, wadlumps[i].name );

		char *asterisk;
		while ( ( asterisk = strchr( outname, '*' ) ) )
		{
			// Half-Lifeify the texture name
			*asterisk = '!';
		}

	//	if ( stbi_write_tga( outname, miptex.width, miptex.height, 3, pic32 ) == 0 )
		{
			Com_Printf( "Failed to write %s\n", outname );
		}

		Z_Free( pic32 );

		Q_sprintf_s( outname, "%s/textures/%s/%s.was", FS_Gamedir(), wadbase, wadlumps[i].name );

		while ( ( asterisk = strchr( outname, '*' ) ) )
		{
			// Half-Lifeify the texture name
			*asterisk = '!';
		}

		was_t was{};
		switch ( wadlumps[i].name[0] )
		{
		case '*':
			was.contents |= CONTENTS_WATER;
			break;
		}

		FILE *scripthandle = fopen( outname, "wb" );
		if ( scripthandle )
		{
			fwrite( &was, sizeof( was ), 1, scripthandle );
			fclose( scripthandle );
		}

	}

	Z_Free( wadlumps );
	fclose( wadhandle );
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
	uint *pPic32 = (uint *)Z_Malloc(c * 4);

	for (int i = 0; i < c; ++i)
	{
		pPic32[i] = d_8to24table[pPic8[i]];
	}

	return (byte *)pPic32;
}

//-------------------------------------------------------------------------------------------------
// Upgrade all WALs in a specified folder to TGAs and WAScripts
//-------------------------------------------------------------------------------------------------
void GL_UpgradeWals_f()
{
	if ( Cmd_Argc() < 2 )
	{
		Com_Printf( "Usage: <folder>\n" );
		return;
	}

	char		folder[MAX_OSPATH];
	const char	*str;

	Q_sprintf_s( folder, "%s/%s/*", FS_Gamedir(), Cmd_Argv( 1 ) );

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

		byte *file = (byte *)Z_Malloc( length );

		fread( file, 1, length, handle );

		fclose( handle );

		int width, height;
		byte *data = LoadWAL( file, length, width, height );
		Z_Free( file );

		char tempname[MAX_QPATH];
		Q_strcpy_s( tempname, str );
		strcpy( strstr( tempname, ".wal" ), ".png" );

		handle = fopen( tempname, "wb" );
		assert( handle );
		img::WritePNG( width, height, true, data, handle );
		fclose( handle );

		Z_Free( data );
	}

	Sys_FindClose();
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
