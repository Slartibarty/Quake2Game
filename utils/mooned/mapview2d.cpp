
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

	setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );

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

#define GridSnap(n, spacing) ((((n) / spacing) * spacing))

static StaticCvar me_maxMapCoord( "me_maxMapCoord", "4096", 0, "The max map coordinate." );

static StaticCvar me_hideSmallGrid( "me_hideSmallGrid", "1", 0, "Whether to hide the small grid." );
static StaticCvar me_gridSpacing( "me_gridSpacing", "16", 0, "Spacing between grid points." );
static StaticCvar me_gridDots( "me_gridDots", "0", 0, "Use dots for the grid." );
static StaticCvar me_gridHigh10( "me_gridHigh10", "1", 0 );
static StaticCvar me_gridHigh64( "me_gridHigh64", "1", 0 );
static StaticCvar me_gridHigh1024( "me_gridHigh1024", "1", 0 );
static StaticCvar me_gridHighSpec( "me_gridHighSpec", "4", 0 );

void MapView2D::DrawGrid( QPainter &painter )
{
	// Check for too small grid
	const int gridSpacing = Clamp( me_gridSpacing.GetInt(), 1, 512 );
	const bool noSmallGrid = ( ( (float)gridSpacing * m_zoomFactor ) < 4.0f ) && me_hideSmallGrid.GetBool();

	int windowWidth = width();
	int windowHeight = height();

	painter.setPen( m_penGrid );

	QPoint ptOrg( horizontalScrollBar()->x(), verticalScrollBar()->y() );

	//
	// Find the window extents snapped to the nearest grid that is just outside the view,
	// and clamped to the legal world coordinates.
	//
	int x1 = GridSnap( ptOrg.x() - gridSpacing, gridSpacing);
	int x2 = x1 + windowWidth / m_zoomFactor + gridSpacing * 3;

	int y1 = GridSnap( ptOrg.y() - gridSpacing, gridSpacing);
	int y2 = y1 + windowHeight / m_zoomFactor + gridSpacing * 3;

	const int maxMapCoord = me_maxMapCoord.GetInt();
	const int minMapCoord = -maxMapCoord;

	x1 = Clamp( x1, minMapCoord, maxMapCoord );
	x2 = Clamp( x2, minMapCoord, maxMapCoord );

	y1 = Clamp( y1, minMapCoord, maxMapCoord );
	y2 = Clamp( y2, minMapCoord, maxMapCoord );

	int customGridSpacing = gridSpacing * me_gridHighSpec.GetBool();

	// No dots if too close together
	const bool gridDots = me_gridDots.GetBool();

	// Draw the vertical grid lines

	bool sel = false;

	for ( int y = y1; y <= y2; y += gridSpacing )
	{
		// Don't draw outside the map bounds
		if ( y > maxMapCoord || y < minMapCoord )
		{
			continue;
		}

		// Highlight the axes
		if ( y == 0 )
		{
			painter.setPen( m_penGrid0 );
			sel = true;
		}
		// Optionally highlight every 1024
		else if ( me_gridHigh1024.GetBool() && ( !( y % 1024 ) ) )
		{
			painter.setPen( m_penGrid1024 );
			sel = true;
		}
		// Optionally highlight every 64
		else if ( me_gridHigh64.GetBool() && ( !( y % 64 ) ) )
		{
			if ( !gridDots )
			{
				painter.setPen( m_penGrid10 );
				sel = true;
			}
		}
		// Optionally highlight every nth grid line
		else if ( me_gridHigh10.GetBool() && ( !( y % customGridSpacing ) ) )
		{
			if ( !gridDots )
			{
				painter.setPen( m_penGrid10 );
				sel = true;
			}
		}

		// Don't draw the base grid if it is too small
		if ( !sel && noSmallGrid )
		{
			continue;
		}

		// Always draw lines for the axes and map boundaries
		if ( !gridDots || sel || y == maxMapCoord || y == minMapCoord )
		{
			painter.drawLine( x1 * m_zoomFactor, y * m_zoomFactor, x2 * m_zoomFactor, y * m_zoomFactor );
		}

		if ( gridDots && !noSmallGrid )
		{
			for ( int xx = x1; xx <= x2; xx += gridSpacing )
			{
				// TODO: Doesn't work
				painter.drawPixmap( xx * m_zoomFactor, y * m_zoomFactor, QPixmap( 1, 1 ) );
			}
		}

		if ( sel )
		{
			painter.setPen( m_penGrid );
			sel = false;
		}
	}

	// Draw the horizontal grid lines

	sel = false;

	for ( int x = x1; x <= x2; x += gridSpacing )
	{
		// Don't draw outside the map bounds
		if ( x > maxMapCoord || x < minMapCoord )
		{
			continue;
		}

		// Highlight the axes
		if ( x == 0 )
		{
			painter.setPen( m_penGrid0 );
			sel = true;
		}
		// Optionally highlight every 1024
		else if ( me_gridHigh1024.GetBool() && ( !( x % 1024 ) ) )
		{
			painter.setPen( m_penGrid1024 );
			sel = true;
		}
		// Optionally highlight every 64
		else if ( me_gridHigh64.GetBool() && ( !( x % 64 ) ) )
		{
			if ( !gridDots )
			{
				painter.setPen( m_penGrid10 );
				sel = true;
			}
		}
		// Optionally highlight every nth grid line
		else if ( me_gridHigh10.GetBool() && ( !( x % customGridSpacing ) ) )
		{
			if ( !gridDots )
			{
				painter.setPen( m_penGrid10 );
				sel = true;
			}
		}

		// Don't draw the base grid if it is too small
		if ( !sel && noSmallGrid )
		{
			continue;
		}

		// Always draw lines for the axes and map boundaries
		if ( !gridDots || sel || x == maxMapCoord || x == minMapCoord )
		{
			painter.drawLine( x * m_zoomFactor, y1 * m_zoomFactor, x * m_zoomFactor, y2 * m_zoomFactor );
		}

		if ( gridDots && !noSmallGrid )
		{
			for ( int yy = y1; yy <= y2; yy += gridSpacing )
			{
				// TODO: Doesn't work
				painter.drawPixmap( x * m_zoomFactor, yy * m_zoomFactor, QPixmap( 1, 1 ) );
			}
		}

		if ( sel )
		{
			painter.setPen( m_penGrid );
			sel = false;
		}
	}

	// Redraw the horizontal axis unless drawing with a dotted grid. This is necessary
	// because it was drawn over by the vertical grid lines
	if ( !gridDots )
	{
		painter.setPen( m_penGrid0 );
		painter.drawLine( x1 * m_zoomFactor, 0, x2 * m_zoomFactor, 0 );
	}
}

void MapView2D::CalcViewExtents()
{
	const int maxMapCoord = me_maxMapCoord.GetInt();
	const int minMapCoord = -maxMapCoord;

	horizontalScrollBar()->setMinimum( minMapCoord * m_zoomFactor - ( height() / 2 ) );
	horizontalScrollBar()->setMaximum( maxMapCoord * m_zoomFactor + ( height() / 2 ) );

	verticalScrollBar()->setMinimum( minMapCoord * m_zoomFactor - ( width() / 2 ) );
	verticalScrollBar()->setMaximum( maxMapCoord * m_zoomFactor + ( width() / 2 ) );
}

void MapView2D::SetZoomFactor( double newZoomFactor )
{
	newZoomFactor = Clamp( newZoomFactor, ZOOM_MIN, ZOOM_MAX );

	if ( newZoomFactor == m_zoomFactor )
	{
		return;
	}

	Com_Printf( "Zoom factor: %g\n", newZoomFactor );

	QPoint pt = QCursor::pos();
	pt = mapToGlobal( pt );
	if ( !rect().contains( pt ) )
	{
		// cursor not in rect, so use the centre
		pt.setX( width() / 2 );
		pt.setY( height() / 2 );
	}

	QPoint ptScroll( horizontalScrollBar()->x(), verticalScrollBar()->y() );

	int posX = ( ptScroll.x() + pt.x() ) / m_zoomFactor;
	int posY = ( ptScroll.y() + pt.y() ) / m_zoomFactor;
	m_zoomFactor = newZoomFactor;
	CalcViewExtents();
	ptScroll.setX( posX - ( pt.x() / m_zoomFactor ) );
	ptScroll.setY( posY - ( pt.y() / m_zoomFactor ) );
	horizontalScrollBar()->scroll( ptScroll.x() * m_zoomFactor, 0 );
	verticalScrollBar()->scroll( ptScroll.y() * m_zoomFactor, 0 );

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

	//painter.setBrush( Qt::SolidPattern );
	//painter.drawRect( rect() );
	//painter.setRenderHint( QPainter::Antialiasing );
	DrawGrid( painter );

	painter.end();
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
