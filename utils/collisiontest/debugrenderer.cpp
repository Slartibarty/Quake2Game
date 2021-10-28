
#include "../../core/core.h"
#include "../../core/sys_includes.h"

#include "GL/glew.h"

#include "SDL2/include/SDL.h"

#include "r_prog.h"

#include "debugrenderer.h"

MyDebugRenderer::MyDebugRenderer()
{
	DebugRenderer::Initialize();

	m_lineSegments.reserve( 4096 );
	m_triangles.reserve( 4096 );

	Shaders_Init();

	//
	// Lines
	//

	glGenVertexArrays( 1, &m_lineVAO );
	glGenBuffers( 1, &m_lineVBO );

	glBindVertexArray( m_lineVAO );
	glBindBuffer( GL_ARRAY_BUFFER, m_lineVBO );

	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( lineVertex_t ), (void *)( 0 ) );
	glVertexAttribPointer( 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( lineVertex_t ), (void *)( 3 * sizeof( GLfloat ) ) );

	//
	// Triangles
	//

}

MyDebugRenderer::~MyDebugRenderer()
{
	Shaders_Shutdown();
}

void MyDebugRenderer::DrawLine( const JPH::Float3 &inFrom, const JPH::Float3 &inTo, JPH::ColorArg inColor )
{
	lineSegment_t &segment = m_lineSegments.emplace_back();

	segment.v1.pos = inFrom;
	segment.v1.col = inColor;

	segment.v2.pos = inTo;
	segment.v2.col = inColor;
}

void MyDebugRenderer::DrawTriangle( JPH::Vec3Arg inV1, JPH::Vec3Arg inV2, JPH::Vec3Arg inV3, JPH::ColorArg inColor )
{
	DrawTriangle_Internal( JPH::Float3( inV1.GetX(), inV1.GetY(), inV1.GetZ() ), JPH::Float3( inV2.GetX(), inV2.GetY(), inV2.GetZ() ), JPH::Float3( inV3.GetX(), inV3.GetY(), inV3.GetZ() ), inColor );
}

MyDebugRenderer::Batch MyDebugRenderer::CreateTriangleBatch( const Triangle *inTriangles, int inTriangleCount )
{
	for ( int i = 0; i < inTriangleCount; ++i )
	{
		const Triangle &triangle = inTriangles[i];
		JPH::Color color( triangle.mV[0].mColor );

		DrawTriangle_Internal( triangle.mV[0].mPosition, triangle.mV[1].mPosition, triangle.mV[2].mPosition, color );
	}

	return nullptr;
}

MyDebugRenderer::Batch MyDebugRenderer::CreateTriangleBatch( const Vertex *inVertices, int inVertexCount, const uint32 *inIndices, int inIndexCount )
{
	return nullptr;
}

void MyDebugRenderer::DrawGeometry( JPH::Mat44Arg inModelMatrix, const JPH::AABox &inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef &inGeometry, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode )
{

}

void MyDebugRenderer::DrawText3D( JPH::Vec3Arg inPosition, const std::string &inString, JPH::ColorArg inColor, float inHeight )
{

}

//=================================================================================================

void MyDebugRenderer::BeginFrame( int width, int height )
{
	m_projMatrix = glm::perspectiveFovRH( DEG2RAD( 90.0f ), (float)width, (float)height, 4.0f, 4096.0f );
	m_viewMatrix = glm::lookAtRH( glm::vec3( 64.0f, 64.0f, 64.0f ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) );

	glClearColor( 0.5f, 0.5f, 0.5f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glUseProgram( glProgs.dbgProg );

	glVertex3f( 0.0f, 64.0f, 128.0f );
}

void MyDebugRenderer::EndFrame( SDL_Window *window )
{
	if ( !m_triangles.empty() )
	{

		m_triangles.clear();
	}

	if ( !m_lineSegments.empty() )
	{
		glUseProgram( glProgs.dbgProg );
		glUniformMatrix4fv( 3, 1, GL_FALSE, (const GLfloat *)&m_viewMatrix );
		glUniformMatrix4fv( 4, 1, GL_FALSE, (const GLfloat *)&m_projMatrix );

		glBindVertexArray( m_lineVAO );
		glBindBuffer( GL_ARRAY_BUFFER, m_lineVBO );

		glBufferData( GL_ARRAY_BUFFER, m_lineSegments.size() * sizeof( lineSegment_t ), m_lineSegments.data(), GL_STREAM_DRAW);

		glDrawArrays( GL_LINES, 0, static_cast<GLsizei>( m_lineSegments.size() * 4 ) );

		glUseProgram( 0 );

		glBindVertexArray( 0 );

		m_lineSegments.clear();
	}

	SDL_GL_SwapWindow( window );
}

//=================================================================================================

void MyDebugRenderer::DrawTriangle_Internal( const JPH::Float3 &v1, const JPH::Float3 &v2, const JPH::Float3 &v3, JPH::Color color )
{
	Triangle &triangle = m_triangles.emplace_back();

	triangle.mV[0].mPosition = v1;
	triangle.mV[0].mColor = color;

	triangle.mV[1].mPosition = v2;
	triangle.mV[1].mColor = color;

	triangle.mV[2].mPosition = v3;
	triangle.mV[2].mColor = color;
}
