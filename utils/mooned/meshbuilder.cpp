
#include "mooned_local.h"

#include "meshbuilder.h"

void LineBuilder_DrawWorldAxes( LineBuilder &bld )
{
	bld.AddLine( vec3( 0.0f, 0.0f, 0.0f ), colors::red, vec3( 100.0f, 0.0f, 0.0f ), colors::red );
	bld.AddLine( vec3( 0.0f, 0.0f, 0.0f ), colors::green, vec3( 0.0f, 100.0f, 0.0f ), colors::green );
	bld.AddLine( vec3( 0.0f, 0.0f, 0.0f ), colors::blue, vec3( 0.0f, 0.0f, 100.0f ), colors::blue );
}

void LineBuilder_DrawCrosshair( LineBuilder &bld, int windowWidth, int windowHeight )
{
	constexpr int crosshairDistHorz = 5;
	constexpr int crosshairDistVert = 6;

	const int centerX = windowWidth / 2;
	const int centerY = windowHeight / 2;

	bld.AddLine(
		vec3( centerX - crosshairDistHorz, centerY - 1, 0 ), colors::black,
		vec3( centerX + crosshairDistHorz + 1, centerY - 1, 0 ), colors::black
	);
	bld.AddLine(
		vec3( centerX - crosshairDistHorz, centerY + 1, 0 ), colors::black,
		vec3( centerX + crosshairDistHorz + 1, centerY + 1, 0 ), colors::black
	);
	bld.AddLine(
		vec3( centerX - 1, centerY - crosshairDistVert, 0 ), colors::black,
		vec3( centerX - 1, centerY + crosshairDistVert, 0 ), colors::black
	);
	bld.AddLine(
		vec3( centerX + 1, centerY - crosshairDistVert, 0 ), colors::black,
		vec3( centerX + 1, centerY + crosshairDistVert, 0 ), colors::black
	);

	bld.AddLine(
		vec3( centerX - crosshairDistHorz, centerY, 0 ), colors::white,
		vec3( centerX + crosshairDistHorz + 1, centerY, 0 ), colors::white
	);
	bld.AddLine(
		vec3( centerX, centerY - crosshairDistVert, 0 ), colors::white,
		vec3( centerX, centerY + crosshairDistVert, 0 ), colors::white
	);
}

void LineBuilder_DrawFrameRect( LineBuilder &bld, int windowWidth, int windowHeight )
{
#if 0
	// Render the world axes.
	//pRender->SetRenderMode( RENDER_MODE_WIREFRAME );
	glDisable( GL_TEXTURE_2D );

	glLineWidth( 4.0f );
	glBegin( GL_LINE_STRIP );

	// Set color
	glColor3ub( 255, 0, 0 );

	glVertex2i( 1, 0 );

	glVertex2i( windowWidth, 0 );

	glVertex2i( windowWidth, windowHeight - 1 );

	glVertex2i( 1, windowHeight - 1 );

	glVertex2i( 1, 0 );

	glEnd();
	glLineWidth( 1.0f );
#endif
}
