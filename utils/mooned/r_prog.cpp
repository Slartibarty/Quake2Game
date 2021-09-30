/*
===================================================================================================

	GLSL shader programs

	Shaders are cached post-link, this is like keeping .obj files around

===================================================================================================
*/

#include "mooned_local.h"

#include "r_local.h"

struct glShaders_t
{
	GLuint dbgVert;
	GLuint dbgFrag;

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

	qgl->glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
	qgl->glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logLength );

	if ( status == GL_TRUE ) {
		Com_Printf( "Successfully compiled \"%s\"\n", filename );
	} else {
		Com_Printf( S_COLOR_RED "ERROR: failed to compile shader \"%s\"\n", filename );
	}

	if ( logLength > 1 ) {
		GLchar *msg = (GLchar *)Mem_StackAlloc( logLength );
		qgl->glGetShaderInfoLog( shader, logLength, nullptr, msg );
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

	GLuint shader = qgl->glCreateShader( type );
	qgl->glShaderSource( shader, 1, &buffer, &bufferLen );

	FileSystem::FreeFile( buffer );

	qgl->glCompileShader( shader );

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
	glShaders.dbgVert = MakeShader( "shaders/mooned/gui.vert", GL_VERTEX_SHADER );
	glShaders.dbgFrag = MakeShader( "shaders/mooned/gui.frag", GL_FRAGMENT_SHADER );

	// create all our programs
	glProgs.dbgProg = qgl->glCreateProgram();
	qgl->glAttachShader( glProgs.dbgProg, glShaders.dbgVert );
	qgl->glAttachShader( glProgs.dbgProg, glShaders.dbgFrag );
	qgl->glLinkProgram( glProgs.dbgProg );

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
	// HACK HACK HACK HACK
	if ( !qgl )
		return;

	qgl->glUseProgram( 0 );

	// values of 0 are silently ignored for glDeleteShader and glDeleteProgram

	qgl->glDeleteProgram( glProgs.dbgProg );

	qgl->glDeleteShader( glShaders.dbgFrag );
	qgl->glDeleteShader( glShaders.dbgVert );
}
