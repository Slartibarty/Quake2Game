
#pragma once

class MapView3D final : public QOpenGLWidget
{
	Q_OBJECT

public:
	explicit MapView3D( QWidget* parent = nullptr );
	~MapView3D() override;

protected:
	void initializeGL() override;
	void resizeGL( int w, int h ) override;
	void paintGL() override;

};
