/*
=============================================================

Misc functions that don't fit anywhere else

=============================================================
*/

#include "gl_local.h"

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
	Com_sprintf (checkname, "%s/scrnshot", ri.FS_Gamedir());
	Sys_Mkdir (checkname);

// 
// find a file name to save it to 
// 
	strcpy(picname,"quake00.tga");

	for (i=0 ; i<=99 ; i++) 
	{ 
		picname[5] = i/10 + '0'; 
		picname[6] = i%10 + '0'; 
		Com_sprintf (checkname, "%s/scrnshot/%s", ri.FS_Gamedir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	} 
	if (i==100) 
	{
		ri.Con_Printf (PRINT_ALL, "SCR_ScreenShot_f: Couldn't create a file\n"); 
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
	ri.Con_Printf (PRINT_ALL, "Wrote %s\n", picname);
}

//-------------------------------------------------------------------------------------------------
// Prints some OpenGL strings
//-------------------------------------------------------------------------------------------------
void GL_Strings_f(void)
{
	ri.Con_Printf(PRINT_ALL, "GL_VENDOR: %s\n", glGetString(GL_VENDOR));
	ri.Con_Printf(PRINT_ALL, "GL_RENDERER: %s\n", glGetString(GL_RENDERER));
	ri.Con_Printf(PRINT_ALL, "GL_VERSION: %s\n", glGetString(GL_VERSION));
	ri.Con_Printf(PRINT_ALL, "GL_GLSL_VERSION: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
//	ri.Con_Printf(PRINT_ALL, "GL_EXTENSIONS: %s\n", gl_config.extensions_string );
}

//-------------------------------------------------------------------------------------------------
// Sets some OpenGL state variables
//-------------------------------------------------------------------------------------------------
void GL_SetDefaultState(void)
{
	glClearColor(1.0f, 0.0f, 0.5f, 1.0f);
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

	GL_TextureMode(gl_texturemode->string);

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
