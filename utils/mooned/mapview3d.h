
#pragma once

#include "r_public.h"

#include "camera.h"
#include "meshbuilder.h"

class MapView3D final : public QOpenGLWidget
{
	Q_OBJECT

private:
	glm::mat4 m_viewMat;
	glm::mat4 m_projMat;

	glProgs_t m_glProgs;

	Camera m_cam;

	LineBuilder m_lineBuilder;
	GLuint m_lineVAO = 0, m_lineVBO = 0;

public:
	explicit MapView3D( QWidget* parent = nullptr );
	~MapView3D() override;

protected:
	void initializeGL() override;
	void resizeGL( int w, int h ) override;
	void paintGL() override;

public:
	void keyPressEvent( QKeyEvent *event ) override;
	void keyReleaseEvent( QKeyEvent *event ) override;

};
