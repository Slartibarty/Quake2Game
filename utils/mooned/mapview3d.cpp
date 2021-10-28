
#include "mooned_local.h"

#include "GL/glew.h"

#include "r_public.h"

#include "mapview3d.h"

static constexpr glm::vec3 g_upVector{ 0.0f, 0.0f, 1.0f };

StaticCvar r_clearColour( "r_clearColour", "0.5 0.5 0.5", 0, "The glClear colour." );

/*
========================
GL_CheckErrors
========================
*/
static void GL_CheckErrors()
{
	const char *msg;
	GLenum err;

	while ( ( err = glGetError() ) != GL_NO_ERROR )
	{
		switch ( err )
		{
		case GL_INVALID_ENUM:
			msg = "GL_INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			msg = "GL_INVALID_VALUE";
			break;
		case GL_INVALID_OPERATION:
			msg = "GL_INVALID_OPERATION";
			break;
		case GL_STACK_OVERFLOW:
			msg = "GL_STACK_OVERFLOW";
			break;
		case GL_STACK_UNDERFLOW:
			msg = "GL_STACK_UNDERFLOW";
			break;
		case GL_OUT_OF_MEMORY:
			msg = "GL_OUT_OF_MEMORY";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			msg = "GL_OUT_OF_MEMORY";
			break;
		case GL_CONTEXT_LOST:						// OpenGL 4.5 or ARB_KHR_robustness
			msg = "GL_OUT_OF_MEMORY";
			break;
		default:
			msg = "UNKNOWN ERROR!";
			break;
		}

		Com_Printf( "GL_CheckErrors: 0x%x - %s\n", err, msg );
	}
}

MapView3D::MapView3D( QWidget* parent )
	: QOpenGLWidget( parent )
{
	// Reserve 128 lines
	m_lineBuilder.Reserve( 128 );
}

MapView3D::~MapView3D()
{

}

void MapView3D::initializeGL()
{
	Shaders_Init( m_glProgs );
}

void MapView3D::resizeGL( int w, int h )
{
	glm::vec3 front;
	front.x = cosf( glm::radians( m_cam.angles.y ) ) * cosf( glm::radians( m_cam.angles.x ) );
	front.y = sinf( glm::radians( m_cam.angles.x ) );
	front.z = sinf( glm::radians( m_cam.angles.y ) ) * cosf( glm::radians( m_cam.angles.x ) );

	m_viewMat = glm::lookAt( m_cam.origin, m_cam.origin + front, g_upVector );
	m_projMat = glm::perspective( glm::radians( 90.0f ), (float)w / (float)h, 0.1f, 4096.0f );
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

		glClearColor( r, g, b, 1.0f );
	}

	R_Clear();

	glUseProgram( m_glProgs.dbgProg );
	GL_CheckErrors();

	glUniformMatrix4fv( 3, 1, GL_FALSE, (const GLfloat *)&m_viewMat );
	glUniformMatrix4fv( 4, 1, GL_FALSE, (const GLfloat *)&m_projMat );

	R_DrawWorldAxes();

	//R_DrawFrameRect( width(), height() );
}

void MapView3D::keyPressEvent( QKeyEvent *event )
{
	switch ( event->key() )
	{
	case Qt::Key_Up:
		m_cam.angles.z += 4.0f;
		break;
	case Qt::Key_Down:
		m_cam.angles.z -= 4.0f;
		break;
	case Qt::Key_Left:
		m_cam.angles.x += 4.0f;
		break;
	case Qt::Key_Right:
		m_cam.angles.x -= 4.0f;
		break;
	}
}

void MapView3D::keyReleaseEvent( QKeyEvent *event )
{

}
