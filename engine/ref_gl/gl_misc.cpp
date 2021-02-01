/*
=============================================================

Misc functions that don't fit anywhere else

=============================================================
*/

#include "gl_local.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

//-------------------------------------------------------------------------------------------------
// Captures a screenshot
//-------------------------------------------------------------------------------------------------
void GL_ScreenShot_f(void)
{
	byte		*buffer;
	char		picname[80]; 
	char		checkname[MAX_OSPATH];
	int			i, c, temp;
	FILE		*f;

	// create the scrnshots directory if it doesn't exist
	Q_sprintf_s (checkname, "%s/scrnshot", FS_Gamedir());
	Sys_Mkdir (checkname);

// 
// find a file name to save it to 
// 
	strcpy(picname,"quake00.tga");

	for (i=0 ; i<=99 ; i++) 
	{ 
		picname[5] = i/10 + '0'; 
		picname[6] = i%10 + '0'; 
		Q_sprintf_s (checkname, "%s/scrnshot/%s", FS_Gamedir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	} 
	if (i==100) 
	{
		Com_Printf("SCR_ScreenShot_f: Couldn't create a file\n"); 
		return;
 	}


	buffer = (byte*)malloc(vid.width*vid.height*3 + 18);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = vid.width&255;
	buffer[13] = vid.width>>8;
	buffer[14] = vid.height&255;
	buffer[15] = vid.height>>8;
	buffer[16] = 24;	// pixel size

	glReadPixels (0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 ); 

	// swap rgb to bgr
	c = 18+vid.width*vid.height*3;
	for (i=18 ; i<c ; i+=3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	f = fopen (checkname, "wb");
	fwrite (buffer, 1, c, f);
	fclose (f);

	free (buffer);
	Com_Printf("Wrote %s\n", picname);
}

//-------------------------------------------------------------------------------------------------
// Prints some OpenGL strings
//-------------------------------------------------------------------------------------------------
void GL_Strings_f(void)
{
	Com_Printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
	Com_Printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
	Com_Printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
	Com_Printf("GL_GLSL_VERSION: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
//	Com_Printf("GL_EXTENSIONS: %s\n", gl_config.extensions_string );
}

//-------------------------------------------------------------------------------------------------
// Extract a WAD file to 24-bit TGA files
//-------------------------------------------------------------------------------------------------
void GL_ExtractWad_f( void )
{
	if ( Cmd_Argc() < 3 )
	{
		Com_Printf( "Usage: <file.wad> <palette.lmp>\n" );
		return;
	}

	char wadbase[MAX_QPATH];
	COM_FileBase( Cmd_Argv( 1 ), wadbase );

	// Create the output folder if it doesn't exist
	Sys_Mkdir( va( "%s/textures/%s", FS_Gamedir(), wadbase ) );

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

	wad2::lumpinfo_t *wadlumps = (wad2::lumpinfo_t *)malloc( sizeof( wad2::lumpinfo_t ) * wadheader.numlumps );

	if ( fread( wadlumps, sizeof( wad2::lumpinfo_t ), wadheader.numlumps, wadhandle ) != wadheader.numlumps )
	{
		free( wadlumps );
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

		pic8 = (byte *)malloc( c );

		fread( pic8, 1, c, wadhandle );

		pic32 = (byte *)malloc( c * 3 );

		for ( int32 pixel = 0; pixel < ( c * 3 ); pixel += 3 )
		{
			pic32[pixel + 0] = palette[pic8[pixel + 0]];
			pic32[pixel + 1] = palette[pic8[pixel + 1]];
			pic32[pixel + 2] = palette[pic8[pixel + 2]];
		}

		free( pic8 );

		char outname[MAX_QPATH];
		Q_sprintf_s( outname, "%s/textures/%s/%s.tga", FS_Gamedir(), wadbase, wadlumps[i].name );

		char *asterisk;
		while ( ( asterisk = strchr( outname, '*' ) ) )
		{
			// Half-Lifeify the texture name
			*asterisk = '!';
		}

		if ( stbi_write_tga( outname, miptex.width, miptex.height, 3, pic32 ) == 0 )
		{
			Com_Printf( "Failed to write %s\n", outname );
		}

		free( pic32 );

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

	free( wadlumps );
	fclose( wadhandle );
}

//-------------------------------------------------------------------------------------------------
// Upgrade all WALs in a specified folder to TGAs and WAScripts
//-------------------------------------------------------------------------------------------------
void GL_UpgradeWals_f( void )
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

		byte *file = (byte *)malloc( length );

		fread( file, 1, length, handle );

		fclose( handle );

		int width, height;
		byte *data = ImageLoaders::LoadWAL( file, length, width, height );
		free( file );

		char tempname[MAX_QPATH];
		Q_strcpy_s( tempname, str );
		strcpy( strstr( tempname, ".wal" ), ".tga" );

		stbi_write_tga( tempname, width, height, 4, data );

		free( data );
	}

	Sys_FindClose();
}
