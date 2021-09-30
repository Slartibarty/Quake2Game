
#include "mooned_local.h"

#include "r_local.h"

static constexpr glm::vec3 g_upVector{ 0.0f, 1.0f, 0.0f };

QOpenGLFunctions_3_3_Compatibility *qgl;

struct rendererLocal_t
{

};

matrices_t g_matrices;

static void InitMatrices()
{
	glm::vec3 origin( 0.0f, 64.0f, 64.0f );
	glm::vec3 angles( 0.0f, 0.0f, 25.0f );

	glm::vec3 front;
	front.x = cosf( glm::radians( angles.y ) ) * cosf( glm::radians( angles.x ) );
	front.y = sinf( glm::radians( angles.x ) );
	front.z = sinf( glm::radians( angles.y ) ) * cosf( glm::radians( angles.x ) );

	g_matrices.view = glm::lookAt( origin, origin + front, g_upVector );
}

void R_Init()
{
	if ( qgl )
	{
		return;
	}

	qgl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Compatibility>();
	qgl->initializeOpenGLFunctions();

	Shaders_Init();
}

void R_Shutdown()
{
	Shaders_Shutdown();
}

