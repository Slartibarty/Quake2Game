/*
===================================================================================================

	GLSL shader programs

	Shaders are cached post-link, this is like keeping .obj files around

===================================================================================================
*/

#include "gl_local.h"

struct glShaders_t
{
	GLuint guiVert;
	GLuint guiFrag;

	GLuint particleVert;
	GLuint particleFrag;

	GLuint smfMeshVert;
	GLuint smfMeshFrag;

	GLuint worldVert;
	GLuint worldFrag;

	GLuint debugMeshVert;
	GLuint debugMeshFrag;

};

static glShaders_t glShaders;

// Globally accessable details
glProgs_t glProgs;

/*
========================
CheckShader
========================
*/
static bool CheckShader( GLuint shader, const char *filename )
{
	GLint status, logLength;

	glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
	glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logLength );

	if ( status == GL_TRUE ) {
		Com_Printf( "Successfully compiled \"%s\"\n", filename );
	} else {
		Com_Printf( S_COLOR_RED "ERROR: failed to compile shader \"%s\"\n", filename );
	}

	if ( logLength > 1 ) {
		GLchar *msg = (GLchar *)Mem_StackAlloc( logLength );
		glGetShaderInfoLog( shader, logLength, nullptr, msg );
		Com_Printf( "Compiler reported: %s", msg );
	}

	return status == GL_TRUE;
}

/*
========================
CheckProgram
========================
*/
static bool CheckProgram( GLuint program )
{
	GLint status, logLength;

	glGetProgramiv( program, GL_LINK_STATUS, &status );
	glGetProgramiv( program, GL_INFO_LOG_LENGTH, &logLength );

	if ( status == GL_TRUE ) {
		Com_Printf( "Successfully linked \"%s\"\n", "N/A" );
	}
	else {
		Com_Printf( S_COLOR_RED "ERROR: failed to link program \"%s\"\n", "N/A" );
	}

	if ( logLength > 1 ) {
		GLchar *msg = (GLchar *)Mem_StackAlloc( logLength );
		glGetProgramInfoLog( program, logLength, nullptr, msg );
		Com_Printf( "Compiler reported: %s", msg );
	}

	return status == GL_TRUE;
}

/*
========================
MakeShader

Loads and compiles a shader from disc
========================
*/
static GLuint MakeShader( const char *filename, GLenum type )
{
	GLchar *buffer;
	GLint bufferLen;

	bufferLen = (GLint)FileSystem::LoadFile( filename, (void **)&buffer );
	if ( !buffer ) {
		return 0;
	}

	GLuint shader = glCreateShader( type );
	glShaderSource( shader, 1, &buffer, &bufferLen );

	FileSystem::FreeFile( buffer );

	glCompileShader( shader );

	if ( !CheckShader( shader, filename ) ) {
		return 0;
	}

	return shader;
}

/*
========================
BuildAllShaders
========================
*/
static void BuildAllShaders()
{
	glShaders.guiVert = MakeShader( "shaders/gui.vert", GL_VERTEX_SHADER );
	glShaders.guiFrag = MakeShader( "shaders/gui.frag", GL_FRAGMENT_SHADER );

	glShaders.particleVert = MakeShader( "shaders/particle.vert", GL_VERTEX_SHADER );
	glShaders.particleFrag = MakeShader( "shaders/particle.frag", GL_FRAGMENT_SHADER );

	glShaders.smfMeshVert = MakeShader( "shaders/smfmesh.vert", GL_VERTEX_SHADER );
	glShaders.smfMeshFrag = MakeShader( "shaders/smfmesh.frag", GL_FRAGMENT_SHADER );

	glShaders.worldVert = MakeShader( "shaders/world.vert", GL_VERTEX_SHADER );
	glShaders.worldFrag = MakeShader( "shaders/world.frag", GL_FRAGMENT_SHADER );

	glShaders.debugMeshVert = MakeShader( "shaders/debugmesh.vert", GL_VERTEX_SHADER );
	glShaders.debugMeshFrag = MakeShader( "shaders/debugmesh.frag", GL_FRAGMENT_SHADER );

	// create all our programs
	glProgs.guiProg = glCreateProgram();
	glAttachShader( glProgs.guiProg, glShaders.guiVert );
	glAttachShader( glProgs.guiProg, glShaders.guiFrag );
	glLinkProgram( glProgs.guiProg );
	CheckProgram( glProgs.guiProg );

	glProgs.particleProg = glCreateProgram();
	glAttachShader( glProgs.particleProg, glShaders.particleVert );
	glAttachShader( glProgs.particleProg, glShaders.particleFrag );
	glLinkProgram( glProgs.particleProg );
	CheckProgram( glProgs.particleProg );

	glProgs.smfMeshProg = glCreateProgram();
	glAttachShader( glProgs.smfMeshProg, glShaders.smfMeshVert );
	glAttachShader( glProgs.smfMeshProg, glShaders.smfMeshFrag );
	glLinkProgram( glProgs.smfMeshProg );
	CheckProgram( glProgs.smfMeshProg );

	glProgs.worldProg = glCreateProgram();
	glAttachShader( glProgs.worldProg, glShaders.worldVert );
	glAttachShader( glProgs.worldProg, glShaders.worldFrag );
	glLinkProgram( glProgs.worldProg );
	CheckProgram( glProgs.worldProg );

	glProgs.debugMeshProg = glCreateProgram();
	glAttachShader( glProgs.debugMeshProg, glShaders.debugMeshVert );
	glAttachShader( glProgs.debugMeshProg, glShaders.debugMeshFrag );
	glLinkProgram( glProgs.debugMeshProg );
	CheckProgram( glProgs.debugMeshProg );
}

/*
========================
RebuildShaders_f
========================
*/
static void R_RebuildShaders_f()
{
	Shaders_Shutdown();
	BuildAllShaders();
}

/*
========================
Shaders_Init
========================
*/
void Shaders_Init()
{
	Cmd_AddCommand( "r_rebuildshaders", R_RebuildShaders_f );

	BuildAllShaders();
}

/*
========================
Shaders_Shutdown
========================
*/
void Shaders_Shutdown()
{
	GL_UseProgram( 0 );

	// values of 0 are silently ignored for glDeleteShader and glDeleteProgram

	glDeleteProgram( glProgs.debugMeshProg );
	glDeleteProgram( glProgs.smfMeshProg );
	glDeleteProgram( glProgs.particleProg );
	glDeleteProgram( glProgs.guiProg );

	glDeleteShader( glShaders.debugMeshVert );
	glDeleteShader( glShaders.debugMeshFrag );

	glDeleteShader( glShaders.smfMeshVert );
	glDeleteShader( glShaders.smfMeshFrag );

	glDeleteShader( glShaders.particleFrag );
	glDeleteShader( glShaders.particleVert );

	glDeleteShader( glShaders.guiFrag );
	glDeleteShader( glShaders.guiVert );
}
