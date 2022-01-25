
#include "mooned_local.h"

#include "meshbuilder.h"

void LineBuilder_DrawWorldAxes( LineBuilder &bld )
{
	bld.AddLine( vec3( 0.0f, 0.0f, 0.0f ), colors::red,   vec3( 100.0f, 0.0f, 0.0f ), colors::red );
	bld.AddLine( vec3( 0.0f, 0.0f, 0.0f ), colors::green, vec3( 0.0f, 100.0f, 0.0f ), colors::green );
	bld.AddLine( vec3( 0.0f, 0.0f, 0.0f ), colors::blue,  vec3( 0.0f, 0.0f, 100.0f ), colors::blue );
}

void LineBuilder_DrawWorldGrid( LineBuilder &bld )
{

}

#if 0
void CRender3D::RenderGrid( int xAxis, int yAxis, float depth, bool bNoSmallGrid )
{
	// Check for too small grid.
	int nGridSpacing = pDoc->GetGridSpacing();

	Color m_clrGrid( Options.colors.clrGrid );
	if ( Options.colors.bScaleGridColor )
	{
		AdjustColorIntensity( m_clrGrid, Options.view2d.iGridIntensity );
	}

	// never allow a grid spacing samller then 2 pixel
	while ( nGridSpacing < 2 )
	{
		nGridSpacing *= 2;
	}

	if ( nGridSpacing < 4 && Options.view2d.bHideSmallGrid )
	{
		bNoSmallGrid = true;
	}

	// No dots if too close together.
	bool bGridDots = Options.view2d.bGridDots ? true : false;
	int iCustomGridSpacing = nGridSpacing * Options.view2d.iGridHighSpec;

	const int xMin = g_MIN_MAP_COORD;
	const int xMax = g_MAX_MAP_COORD;
	const int yMin = g_MIN_MAP_COORD;
	const int yMax = g_MAX_MAP_COORD;

	Assert( xMin < xMax );
	Assert( yMin < yMax );

	// Draw the vertical grid lines.

	Vector vPointMin( depth, depth, depth );
	Vector vPointMax( depth, depth, depth );

	vPointMin[xAxis] = xMin;
	vPointMax[xAxis] = xMax;

	auto PickGridHighlight = [this]( bool bGridDots, int iCustomGridSpacing, int nGridLine ) -> bool
	{
		Color m_clrAxis( Options.colors.clrAxis );
		if ( Options.colors.bScaleAxisColor )
		{
			AdjustColorIntensity( m_clrAxis, Options.view2d.iGridIntensity );
		}
		Color m_clrGrid1024( Options.colors.clrGrid1024 );
		if ( Options.colors.bScaleGrid1024Color )
		{
			AdjustColorIntensity( m_clrGrid1024, Options.view2d.iGridIntensity );
		}
		Color m_clrGridCustom( Options.colors.clrGrid10 );
		if ( Options.colors.bScaleGrid10Color )
		{
			AdjustColorIntensity( m_clrGridCustom, Options.view2d.iGridIntensity );
		}

		if ( nGridLine == 0 )
		{
			SetDrawColor( m_clrAxis );
			return true;
		}
		//
		// Optionally highlight every 1024.
		//
		if ( Options.view2d.bGridHigh1024 && ( !( nGridLine % 1024 ) ) )
		{
			SetDrawColor( m_clrGrid1024 );
			return true;
		}
		//
		// Optionally highlight every 64.
		//
		else if ( Options.view2d.bGridHigh64 && ( !( nGridLine % 64 ) ) )
		{
			if ( !bGridDots )
			{
				SetDrawColor( m_clrGridCustom );
				return true;
			}
		}
		//
		// Optionally highlight every nth grid line.
		//

		if ( Options.view2d.bGridHigh10 && ( !( nGridLine % iCustomGridSpacing ) ) )
		{
			if ( !bGridDots )
			{
				SetDrawColor( m_clrGridCustom );
				return true;
			}
		}

		return false;
	};

	for ( int y = yMin; y <= yMax; y += nGridSpacing )
	{
		SetDrawColor( m_clrGrid );

		bool bHighligh = PickGridHighlight( bGridDots, iCustomGridSpacing, y );

		// Don't draw the base grid if it is too small.

		if ( !bHighligh && bNoSmallGrid ) {
			continue;
		}

		// Always draw lines for the axes and map boundaries.

		if ( ( !bGridDots ) || bHighligh || ( y == g_MAX_MAP_COORD ) || ( y == g_MIN_MAP_COORD ) )
		{
			vPointMin[yAxis] = vPointMax[yAxis] = y;
			DrawLine( vPointMin, vPointMax );
		}
	}

	vPointMin[yAxis] = yMin;
	vPointMax[yAxis] = yMax;

	for ( int x = xMin; x <= xMax; x += nGridSpacing )
	{
		SetDrawColor( m_clrGrid );

		bool bHighligh = PickGridHighlight( bGridDots, iCustomGridSpacing, x );

		// Don't draw the base grid if it is too small.
		if ( !bHighligh && bNoSmallGrid ) {
			continue;
		}

		// Always draw lines for the axes and map boundaries.

		if ( ( !bGridDots ) || bHighligh || ( x == g_MAX_MAP_COORD ) || ( x == g_MIN_MAP_COORD ) )
		{
			vPointMin[xAxis] = vPointMax[xAxis] = x;
			DrawLine( vPointMin, vPointMax );
		}
	}
}
#endif

void LineBuilder_DrawCrosshair( LineBuilder &bld, int windowWidth, int windowHeight )
{
	constexpr int crosshairDistHorz = 5;
	constexpr int crosshairDistVert = 6;

	const int centerX = windowWidth / 2;
	const int centerY = windowHeight / 2;

	bld.AddLine(
		vec3( centerX - crosshairDistHorz,     centerY - 1, 0 ), colors::black,
		vec3( centerX + crosshairDistHorz + 1, centerY - 1, 0 ), colors::black
	);
	bld.AddLine(
		vec3( centerX - crosshairDistHorz,     centerY + 1, 0 ), colors::black,
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
		vec3( centerX - crosshairDistHorz,     centerY, 0 ), colors::white,
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
