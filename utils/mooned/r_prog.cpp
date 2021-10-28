/*
===================================================================================================

	GLSL shader programs

	Shaders are cached post-link, this is like keeping .obj files around

===================================================================================================
*/

#include "mooned_local.h"
#include "r_local.h"

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
static void BuildAllShaders( glProgs_t &glProgs )
{
	GLuint dbgVert = MakeShader( "shaders/mooned/debug.vert", GL_VERTEX_SHADER );			//!!			// LEAK!!!
	GLuint dbgFrag = MakeShader( "shaders/mooned/debug.frag", GL_FRAGMENT_SHADER );			//!!			// LEAK!!!

	// create all our programs
	glProgs.dbgProg = glCreateProgram();
	glAttachShader( glProgs.dbgProg, dbgVert );
	glAttachShader( glProgs.dbgProg, dbgFrag );
	glLinkProgram( glProgs.dbgProg );

}

/*
========================
Shaders_Init
========================
*/
void Shaders_Init( glProgs_t &glProgs )
{
	BuildAllShaders( glProgs );
}

/*
========================
Shaders_Shutdown
========================
*/
void Shaders_Shutdown( glProgs_t &glProgs )
{
	glUseProgram( 0 );

	// values of 0 are silently ignored for glDeleteShader and glDeleteProgram

	glDeleteProgram( glProgs.dbgProg );

//	glDeleteShader( glShaders.dbgFrag );
//	glDeleteShader( glShaders.dbgVert );
}
