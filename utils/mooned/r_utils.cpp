
#include "mooned_local.h"

#include "r_local.h"

#define CROSSHAIR_DIST_HORIZONTAL		5
#define CROSSHAIR_DIST_VERTICAL			6

void R_Clear()
{
	qgl->glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void R_DrawWorldAxes()
{
	qgl->glBegin( GL_LINES );

	qgl->glColor3ub( 255, 0, 0 );
	qgl->glVertex3f( 0, 0, 0 );

	qgl->glVertex3f( 100, 0, 0 );

	qgl->glColor3ub( 0, 255, 0 );
	qgl->glVertex3f( 0, 0, 0 );

	qgl->glVertex3f( 0, 100, 0 );

	qgl->glColor3ub( 0, 0, 255 );
	qgl->glVertex3f( 0, 0, 0 );

	qgl->glVertex3f( 0, 0, 100 );

	qgl->glEnd();
}

void R_DrawCrosshair( int windowWidth, int windowHeight )
{
	int nCenterX;
	int nCenterY;

	nCenterX = windowWidth / 2;
	nCenterY = windowHeight / 2;

	// Render the world axes
	//pRender->SetRenderMode( RENDER_MODE_WIREFRAME );
	qgl->glDisable( GL_TEXTURE_2D );

	qgl->glBegin( GL_LINES );

	qgl->glColor3ub( 0, 0, 0 );
	qgl->glVertex3f( nCenterX - CROSSHAIR_DIST_HORIZONTAL, nCenterY - 1, 0 );
	qgl->glVertex3f( nCenterX + CROSSHAIR_DIST_HORIZONTAL + 1, nCenterY - 1, 0 );

	qgl->glVertex3f( nCenterX - CROSSHAIR_DIST_HORIZONTAL, nCenterY + 1, 0 );
	qgl->glVertex3f( nCenterX + CROSSHAIR_DIST_HORIZONTAL + 1, nCenterY + 1, 0 );

	qgl->glVertex3f( nCenterX - 1, nCenterY - CROSSHAIR_DIST_VERTICAL, 0 );
	qgl->glVertex3f( nCenterX - 1, nCenterY + CROSSHAIR_DIST_VERTICAL, 0 );

	qgl->glVertex3f( nCenterX + 1, nCenterY - CROSSHAIR_DIST_VERTICAL, 0 );
	qgl->glVertex3f( nCenterX + 1, nCenterY + CROSSHAIR_DIST_VERTICAL, 0 );

	qgl->glColor3ub( 255, 255, 255 );
	qgl->glVertex3f( nCenterX - CROSSHAIR_DIST_HORIZONTAL, nCenterY, 0 );
	qgl->glVertex3f( nCenterX + CROSSHAIR_DIST_HORIZONTAL + 1, nCenterY, 0 );

	qgl->glVertex3f( nCenterX, nCenterY - CROSSHAIR_DIST_VERTICAL, 0 );
	qgl->glVertex3f( nCenterX, nCenterY + CROSSHAIR_DIST_VERTICAL, 0 );

	qgl->glEnd();
}

void R_DrawFrameRect( int windowWidth, int windowHeight )
{
	// Render the world axes.
	//pRender->SetRenderMode( RENDER_MODE_WIREFRAME );
	qgl->glDisable( GL_TEXTURE_2D );

	qgl->glLineWidth( 4.0f );
	qgl->glBegin( GL_LINE_STRIP );

	// Set color
	qgl->glColor3ub( 255, 0, 0 );

#if 0

	qgl->glVertex2i( 1, 0 );

	qgl->glVertex2i( windowWidth, 0 );

	qgl->glVertex2i( windowWidth, windowHeight - 1 );

	qgl->glVertex2i( 1, windowHeight - 1 );

	qgl->glVertex2i( 1, 0 );

#else

	qgl->glVertex2f( 0.0f, 0.0f );
	qgl->glVertex2f( 0.0f, 1.0f );
	qgl->glVertex2f( 1.0f, 1.0f );
	qgl->glVertex2f( 0.0f, 1.0f );
	qgl->glVertex2f( 0.0f, 0.0f );

#endif

	qgl->glEnd();
	qgl->glLineWidth( 1.0f );
}
