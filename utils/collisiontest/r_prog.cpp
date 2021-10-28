/*
===================================================================================================

	GLSL shader programs

	Shaders are cached post-link, this is like keeping .obj files around

===================================================================================================
*/

#include "../../core/core.h"

#include "../../framework/framework_public.h"

#include "GL/glew.h"

#include "r_prog.h"

static struct glShaders_t
{
	GLuint dbgVert;
	GLuint dbgFrag;

	GLuint dbgPolyVert;
	GLuint dbgPolyFrag;

} glShaders;

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
	glShaders.dbgVert = MakeShader( "shaders/collisiontest/debug.vert", GL_VERTEX_SHADER );
	glShaders.dbgFrag = MakeShader( "shaders/collisiontest/debug.frag", GL_FRAGMENT_SHADER );

	glShaders.dbgPolyVert = MakeShader( "shaders/collisiontest/debugpoly.vert", GL_VERTEX_SHADER );

	// create all our programs
	glProgs.dbgProg = glCreateProgram();
	glAttachShader( glProgs.dbgProg, glShaders.dbgVert );
	glAttachShader( glProgs.dbgProg, glShaders.dbgFrag );
	glLinkProgram( glProgs.dbgProg );

	glProgs.dbgPolyProg = glCreateProgram();
	glAttachShader( glProgs.dbgPolyProg, glShaders.dbgPolyVert );
	glAttachShader( glProgs.dbgPolyProg, glShaders.dbgFrag );
	glLinkProgram( glProgs.dbgPolyProg );

}

/*
========================
RebuildShaders_f
========================
*/
CON_COMMAND( r_rebuildShaders, "Rebuild shaders.", 0 )
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
	BuildAllShaders();
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

	glDeleteProgram( glProgs.dbgProg );

	glDeleteShader( glShaders.dbgFrag );
	glDeleteShader( glShaders.dbgVert );
}
