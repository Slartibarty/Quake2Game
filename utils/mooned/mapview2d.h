
#pragma once

class MapView2D final : public QFrame
{
	Q_OBJECT

private:
	double m_zoomFactor = 2.0;			// zoom factor (* map units)
	int m_xPos = 0, m_yPos = 0;

	QPen m_penGrid;
	QPen m_penGrid0;
	QPen m_penGrid10;
	QPen m_penGrid1024;

	bool m_mouseDown = false;

	bool HighlightGridLine( QPainter &painter, int gridLine, int customGridSpacing );
	void DrawGrid( QPainter &painter );

	void CalcViewExtents();
	void SetZoomFactor( double newFactor );
	void ZoomIn();
	void ZoomOut();

public:
	explicit MapView2D( QWidget *parent = nullptr );
	~MapView2D() override;

protected:
	void paintEvent( QPaintEvent *event ) override;
	void mousePressEvent( QMouseEvent *event ) override;
	void mouseReleaseEvent( QMouseEvent *event ) override;
	void mouseDoubleClickEvent( QMouseEvent *event ) override;
	void mouseMoveEvent( QMouseEvent *event ) override;
	void wheelEvent( QWheelEvent *event ) override;

};
