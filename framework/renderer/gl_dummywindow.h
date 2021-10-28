/*
===================================================================================================

	OpenGL Dummy Window

===================================================================================================
*/

#pragma once

#ifdef _WIN32

struct DummyVars
{
	void *	hWnd;
	void *	hGLRC;
};

void WGLimp_CreateDummyWindow( DummyVars &dvars );
void WGLimp_DestroyDummyWindow( DummyVars &dvars );

#endif
