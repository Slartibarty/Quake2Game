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
	Q_sprintf_s (checkname, "%s/scrnshot", RI_FS_Gamedir());
	Sys_Mkdir (checkname);

// 
// find a file name to save it to 
// 
	strcpy(picname,"quake00.tga");

	for (i=0 ; i<=99 ; i++) 
	{ 
		picname[5] = i/10 + '0'; 
		picname[6] = i%10 + '0'; 
		Q_sprintf_s (checkname, "%s/scrnshot/%s", RI_FS_Gamedir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	} 
	if (i==100) 
	{
		RI_Com_Printf("SCR_ScreenShot_f: Couldn't create a file\n"); 
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
	RI_Com_Printf("Wrote %s\n", picname);
}

//-------------------------------------------------------------------------------------------------
// Prints some OpenGL strings
//-------------------------------------------------------------------------------------------------
void GL_Strings_f(void)
{
	RI_Com_Printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
	RI_Com_Printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
	RI_Com_Printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
//	RI_Com_Printf("GL_GLSL_VERSION: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
//	RI_Com_Printf("GL_EXTENSIONS: %s\n", gl_config.extensions_string );
}

//-------------------------------------------------------------------------------------------------
// Sets some OpenGL state variables
// Called once by R_Init
//-------------------------------------------------------------------------------------------------
void GL_SetDefaultState(void)
{
	glClearColor( DEFAULT_CLEARCOLOR );
	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.666f);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glShadeModel(GL_FLAT);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_TexEnv(GL_REPLACE);

	if (GLEW_EXT_point_parameters && gl_ext_pointparameters->value)
	{
		float attenuations[3];

		attenuations[0] = gl_particle_att_a->value;
		attenuations[1] = gl_particle_att_b->value;
		attenuations[2] = gl_particle_att_c->value;

		glEnable(GL_POINT_SMOOTH);
		glPointParameterfEXT(GL_POINT_SIZE_MIN_EXT, gl_particle_min_size->value);
		glPointParameterfEXT(GL_POINT_SIZE_MAX_EXT, gl_particle_max_size->value);
		glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, attenuations);
	}
}

//-------------------------------------------------------------------------------------------------
// Extract a WAD file to 24-bit TGA files
//-------------------------------------------------------------------------------------------------
void GL_ExtractWad_f( void )
{
	if ( RI_Cmd_Argc() < 3 )
	{
		RI_Com_Printf( "Usage: <file.wad> <palette.lmp>\n" );
		return;
	}

	char wadbase[MAX_QPATH];
	COM_FileBase( RI_Cmd_Argv( 1 ), wadbase );

	// Create the output folder if it doesn't exist
	Sys_Mkdir( va( "%s/textures/%s", RI_FS_Gamedir(), wadbase ) );

	FILE *wadhandle;

	wadhandle = fopen( va( "%s/%s", RI_FS_Gamedir(), RI_Cmd_Argv( 2 ) ), "rb" );
	if ( !wadhandle )
	{
		RI_Com_Printf( "Couldn't open %s\n", RI_Cmd_Argv( 2 ) );
		return;
	}

	byte palette[256 * 3];

	if ( fread( palette, 3, 256, wadhandle ) != 256 )
	{
		fclose( wadhandle );
		RI_Com_Printf( "Malformed palette lump\n" );
	}

	fclose( wadhandle );

	wadhandle = fopen( va( "%s/%s", RI_FS_Gamedir(), RI_Cmd_Argv( 1 ) ), "rb" );
	if ( !wadhandle )
	{
		RI_Com_Printf( "Couldn't open %s\n", RI_Cmd_Argv( 1 ) );
		return;
	}

	wad2::wadinfo_t wadheader;

	if ( fread( &wadheader, sizeof( wadheader ), 1, wadhandle ) != 1 || wadheader.identification != wad2::IDWADHEADER )
	{
		fclose( wadhandle );
		RI_Com_Printf( "Malformed wadfile\n" );
		return;
	}

	fseek( wadhandle, wadheader.infotableofs, SEEK_SET );

	wad2::lumpinfo_t *wadlumps = (wad2::lumpinfo_t *)malloc( sizeof( wad2::lumpinfo_t ) * wadheader.numlumps );

	if ( fread( wadlumps, sizeof( wad2::lumpinfo_t ), wadheader.numlumps, wadhandle ) != wadheader.numlumps )
	{
		free( wadlumps );
		fclose( wadhandle );
		RI_Com_Printf( "Malformed wadfile\n" );
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
		Q_sprintf_s( outname, "%s/textures/%s/%s.tga", RI_FS_Gamedir(), wadbase, wadlumps[i].name );

		char *asterisk;
		while ( ( asterisk = strchr( outname, '*' ) ) )
		{
			// Half-Lifeify the texture name
			*asterisk = '!';
		}

		if ( stbi_write_tga( outname, miptex.width, miptex.height, 3, pic32 ) == 0 )
		{
			RI_Com_Printf( "Failed to write %s\n", outname );
		}

		free( pic32 );

		Q_sprintf_s( outname, "%s/textures/%s/%s.was", RI_FS_Gamedir(), wadbase, wadlumps[i].name );

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
	if ( RI_Cmd_Argc() < 2 )
	{
		RI_Com_Printf( "Usage: <folder>\n" );
		return;
	}

	char		folder[MAX_OSPATH];
	const char	*str;

	Q_sprintf_s( folder, "%s/%s", RI_FS_Gamedir(), RI_Cmd_Argv( 1 ) );

	str = Sys_FindFirst( folder, 0, 0 );
	if ( !str ) {
		RI_Com_Printf( "No files found in folder\n" );
		Sys_FindClose();
		return;
	}

	for ( ; str; str = Sys_FindNext( 0, 0 ) )
	{
		if ( strstr( str, ".wal" ) == NULL )
			continue;
		RI_Com_Printf( "Upgrading %s\n", str );

	}

	Sys_FindClose();

}
