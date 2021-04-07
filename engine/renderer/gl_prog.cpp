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
		Com_Printf( "ERROR: failed to compile shader \"%s\"\n", filename );
	}

	if ( logLength > 1 ) {
		GLchar *msg = (GLchar *)Z_StackAlloc( logLength );
		glGetShaderInfoLog( shader, logLength, nullptr, msg );
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

	bufferLen = (GLint)FS_LoadFile( filename, (void **)&buffer );
	if ( !buffer ) {
		return 0;
	}

	GLuint shader = glCreateShader( type );
	glShaderSource( shader, 1, &buffer, &bufferLen );

	FS_FreeFile( buffer );

	glCompileShader( shader );

	if ( !CheckShader( shader, filename ) ) {
		return 0;
	}

	return shader;
}

/*
========================
Shaders_Init
========================
*/
void Shaders_Init()
{
	// create all our shaders
	glShaders.guiVert = MakeShader( "shaders/gui.vert", GL_VERTEX_SHADER );
	glShaders.guiFrag = MakeShader( "shaders/gui.frag", GL_FRAGMENT_SHADER );

	glShaders.particleVert = MakeShader( "shaders/particle.vert", GL_VERTEX_SHADER );
	glShaders.particleFrag = MakeShader( "shaders/particle.frag", GL_FRAGMENT_SHADER );

	// create all our programs
	glProgs.guiProg = glCreateProgram();
	glAttachShader( glProgs.guiProg, glShaders.guiVert );
	glAttachShader( glProgs.guiProg, glShaders.guiFrag );
	glLinkProgram( glProgs.guiProg );

	glProgs.particleProg = glCreateProgram();
	glAttachShader( glProgs.particleProg, glShaders.particleVert );
	glAttachShader( glProgs.particleProg, glShaders.particleFrag );
	glLinkProgram( glProgs.particleProg );
}

/*
========================
Shaders_Shutdown
========================
*/
void Shaders_Shutdown()
{
	glUseProgram( 0 );

	// values of 0 are silently ignored for glDeleteShader and glDeleteProgram

	glDeleteShader( glShaders.particleFrag );
	glDeleteShader( glShaders.particleVert );

	glDeleteShader( glShaders.guiFrag );
	glDeleteShader( glShaders.guiVert );

	glDeleteProgram( glProgs.particleProg );
	glDeleteProgram( glProgs.guiProg );
}
