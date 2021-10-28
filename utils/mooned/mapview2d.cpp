
#include "mooned_local.h"

#include "mapview2d.h"

#define ZOOM_MIN	0.02125
#define ZOOM_MAX	256.0

//=================================================================================================
// public

MapView2D::MapView2D( QWidget *parent )
{
	/*QGradientStop stop1{1.0, QColor(0, 0, 0)};
	QGradientStop stop2{ 0.0, QColor( 255, 255, 255 ) };

	QGradientStops stops{ stop1, stop2 };*/

	//m_bgGradient.setStops( stops );

	m_penGrid.setColor( QColor( 50, 50, 50 ) );
	m_penGrid0.setColor( QColor( 0, 100, 100 ) );
	m_penGrid10.setColor( QColor( 40, 40, 40 ) );
	m_penGrid1024.setColor( QColor( 40, 40, 40 ) );
}

MapView2D::~MapView2D()
{

}

//=================================================================================================
// private

static StaticCvar me_maxMapCoord( "me_maxMapCoord", "4096", 0, "The max map coordinate." );

static StaticCvar me_hideSmallGrid( "me_hideSmallGrid", "1", 0, "Whether to hide the small grid." );
static StaticCvar me_gridSpacing( "me_gridSpacing", "64", 0, "Spacing between grid points." );
static StaticCvar me_gridDots( "me_gridDots", "0", 0, "Use dots for the grid." );
static StaticCvar me_gridHigh10( "me_gridHigh10", "1", 0 );
static StaticCvar me_gridHigh64( "me_gridHigh64", "1", 0 );
static StaticCvar me_gridHigh1024( "me_gridHigh1024", "1", 0 );
static StaticCvar me_gridHighSpec( "me_gridHighSpec", "4", 0 );

static constexpr int SnapToGrid( int line, int grid )
{
	return line - ( line % grid );
}

bool MapView2D::HighlightGridLine( QPainter &painter, int gridLine, int customGridSpacing )
{
	// Highlight the axes
	if ( gridLine == 0 )
	{
		painter.setPen( m_penGrid0 );
		return true;
	}
	// Optionally highlight every 1024
	else if ( me_gridHigh1024.GetBool() && ( !( gridLine % 1024 ) ) )
	{
		painter.setPen( m_penGrid1024 );
		return true;
	}
	// Optionally highlight every 64
	else if ( me_gridHigh64.GetBool() && ( !( gridLine % 64 ) ) )
	{
		painter.setPen( m_penGrid10 );
		return true;
	}
	// Optionally highlight every nth grid line
	else if ( me_gridHigh10.GetBool() && ( !( gridLine % customGridSpacing ) ) )
	{
		painter.setPen( m_penGrid10 );
		return true;
	}

	return false;
}

void MapView2D::DrawGrid( QPainter &painter )
{
	// Check for too small grid
	const int gridSpacing = Clamp( me_gridSpacing.GetInt(), 1, 512 );
	const bool noSmallGrid = ( ( (float)gridSpacing * m_zoomFactor ) < 4.0f ) && me_hideSmallGrid.GetBool();

	int windowWidth = width();
	int windowHeight = height();

	painter.fillRect( 0, 0, windowWidth, windowHeight, Qt::black );

	painter.setPen( m_penGrid );

	//
	// Find the window extents snapped to the nearest grid that is just outside the view,
	// and clamped to the legal world coordinates.
	//
	int xMin = SnapToGrid( m_xPos - gridSpacing, gridSpacing );
	int xMax = xMin + windowWidth / m_zoomFactor + gridSpacing * 3;

	int yMin = SnapToGrid( m_yPos - gridSpacing, gridSpacing );
	int yMax = yMin + windowHeight / m_zoomFactor + gridSpacing * 3;

	const int maxMapCoord = me_maxMapCoord.GetInt();
	const int minMapCoord = -maxMapCoord;

	xMin = Clamp( xMin, minMapCoord, maxMapCoord );
	xMax = Clamp( xMax, minMapCoord, maxMapCoord );

	yMin = Clamp( yMin, minMapCoord, maxMapCoord );
	yMax = Clamp( yMax, minMapCoord, maxMapCoord );

	assert( xMin < xMax );
	assert( yMin < yMax );

	int customGridSpacing = gridSpacing * me_gridHighSpec.GetInt();

	// No dots if too close together
	//const bool gridDots = me_gridDots.GetBool();

	// Draw the vertical grid lines

	for ( int y = yMin; y <= yMax; y += gridSpacing )
	{
		// Don't draw outside the map bounds
		if ( y > maxMapCoord || y < minMapCoord )
		{
			continue;
		}

		bool changedPen = HighlightGridLine( painter, y, customGridSpacing );

		// Don't draw the base grid if it is too small
		if ( !changedPen && noSmallGrid )
		{
			continue;
		}

		// Always draw lines for the axes and map boundaries
		if ( changedPen || y == maxMapCoord || y == minMapCoord )
		{
			painter.drawLine( xMin * m_zoomFactor, y * m_zoomFactor, xMax * m_zoomFactor, y * m_zoomFactor );
		}

		if ( changedPen )
		{
			painter.setPen( m_penGrid );
			changedPen = false;
		}
	}

	// Draw the horizontal grid lines

	for ( int x = xMin; x <= xMax; x += gridSpacing )
	{
		// Don't draw outside the map bounds
		if ( x > maxMapCoord || x < minMapCoord )
		{
			continue;
		}

		bool changedPen = HighlightGridLine( painter, x, customGridSpacing );

		// Don't draw the base grid if it is too small
		if ( !changedPen && noSmallGrid )
		{
			continue;
		}

		// Always draw lines for the axes and map boundaries
		if ( changedPen || x == maxMapCoord || x == minMapCoord )
		{
			painter.drawLine( x * m_zoomFactor, yMin * m_zoomFactor, x * m_zoomFactor, yMax * m_zoomFactor );
		}

		if ( changedPen )
		{
			painter.setPen( m_penGrid );
		}
	}

	// Redraw the horizontal axis unless drawing with a dotted grid. This is necessary
	// because it was drawn over by the vertical grid lines
	painter.setPen( m_penGrid0 );
	painter.drawLine( xMin * m_zoomFactor, 0, xMax * m_zoomFactor, 0 );
}

void MapView2D::CalcViewExtents()
{
	const int maxMapCoord = me_maxMapCoord.GetInt();
	const int minMapCoord = -maxMapCoord;
}

void MapView2D::SetZoomFactor( double newZoomFactor )
{
	newZoomFactor = Clamp( newZoomFactor, ZOOM_MIN, ZOOM_MAX );

	if ( newZoomFactor == m_zoomFactor )
	{
		return;
	}

	Com_Printf( "Zoom factor: %g\n", newZoomFactor );

	QPoint cursorPt = QCursor::pos();
	cursorPt = mapToGlobal( cursorPt );
	if ( !rect().contains( cursorPt ) )
	{
		// cursor not in rect, so use the centre
		cursorPt.setX( width() / 2 );
		cursorPt.setY( height() / 2 );
	}

	int posX = ( m_xPos + cursorPt.x() ) / m_zoomFactor;
	int posY = ( m_yPos + cursorPt.y() ) / m_zoomFactor;
	m_zoomFactor = newZoomFactor;
	CalcViewExtents();
	m_xPos = posX - ( cursorPt.x() / m_zoomFactor );
	m_yPos = posY - ( cursorPt.y() / m_zoomFactor );

	update();
}

void MapView2D::ZoomIn()
{
	SetZoomFactor( m_zoomFactor * 1.2 );
}

void MapView2D::ZoomOut()
{
	SetZoomFactor( m_zoomFactor / 1.2 );
}

//=================================================================================================
// protected

void MapView2D::paintEvent( QPaintEvent *event )
{
	QPainter painter;
	painter.begin( this );

	DrawGrid( painter );

	painter.end();
}

void MapView2D::mousePressEvent( QMouseEvent *event )
{
	m_mouseDown = true;
}

void MapView2D::mouseReleaseEvent( QMouseEvent *event )
{
	m_mouseDown = false;
}

void MapView2D::mouseDoubleClickEvent( QMouseEvent *event )
{

}

void MapView2D::mouseMoveEvent( QMouseEvent *event )
{
	if ( m_mouseDown )
	{
		// TODO
	}
}

void MapView2D::wheelEvent( QWheelEvent *event )
{
	if ( event->angleDelta().y() > 0 ) {
		ZoomIn();
	} else {
		assert( event->angleDelta().y() < 0);
		ZoomOut();
	}
}
