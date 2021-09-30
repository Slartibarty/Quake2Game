
#include "mooned_local.h"

#include "r_public.h"

#include "mapview3d.h"

StaticCvar r_clearColour( "r_clearColour", "0.5 0.5 0.5", 0, "The glClear colour." );

MapView3D::MapView3D( QWidget* parent )
	: QOpenGLWidget( parent )
{

}

MapView3D::~MapView3D()
{

}

void MapView3D::initializeGL()
{
	R_Init();
}

void MapView3D::resizeGL( int w, int h )
{
	g_matrices.projection = glm::perspective( glm::radians( 90.0f ), (float)w / (float)h, 0.1f, 4096.0f );
}

void MapView3D::paintGL()
{
	if ( r_clearColour.IsModified() )
	{
		r_clearColour.ClearModified();

		float r, g, b;
		if ( sscanf( r_clearColour.GetString(), "%f %f %f", &r, &g, &b ) != 3 )
		{
			return;
		}

		qgl->glClearColor( r, g, b, 1.0f );
	}

	R_Clear();

	qgl->glUseProgram( glProgs.dbgProg );

	R_DrawWorldAxes();

	//R_DrawFrameRect( width(), height() );
}
