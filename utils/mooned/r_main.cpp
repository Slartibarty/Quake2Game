
#include "mooned_local.h"
#include "r_local.h"

#include "../../framework/renderer/gl_dummywindow.h"

#include "GL/wglew.h"

struct rendererLocal_t
{

};

void R_Init()
{
	DummyVars dvars;
	WGLimp_CreateDummyWindow( dvars );

	// initialize our OpenGL dynamic bindings....
	glewExperimental = GL_TRUE;
	if ( glewInit() != GLEW_OK || wglewInit() != GLEW_OK )
	{
		Com_FatalError( "Failed to retrieve OpenGL bindings\n" );
	}

	WGLimp_DestroyDummyWindow( dvars );
}

void R_Shutdown()
{

}
