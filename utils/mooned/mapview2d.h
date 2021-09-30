
#pragma once

class MapView2D final : public QAbstractScrollArea
{
	Q_OBJECT

private:
	double m_zoomFactor = 2.0;			// zoom factor (* map units)
	int m_xScroll = 0, m_yScroll = 0;

	QGradient m_bgGradient;
	QPen m_penGrid;
	QPen m_penGrid0;
	QPen m_penGrid10;
	QPen m_penGrid1024;

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
	void wheelEvent( QWheelEvent *event ) override;

};
