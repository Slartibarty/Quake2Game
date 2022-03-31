
#include "gl_local.h"

#ifndef NO_JOLT_DEBUG

#include <Jolt/Jolt.h>

#include <Jolt/Physics/PhysicsSystem.h>

#include <Jolt/Renderer/DebugRenderer.h>
#include <Jolt/Renderer/DebugRendererRecorder.h>

#include "../shared/physics.h"

#define JOLT2QUAKE_FACTOR		39.3700787402f		// Factor to convert meters to game units

static StaticCvar r_drawJolt( "r_drawJolt", "0", 0, "Controls Jolt debug drawing." );

#if 1

class MyDebugRenderer final : public JPH::DebugRenderer
{
public:
	MyDebugRenderer();
	~MyDebugRenderer() override;

	void DrawLine( const JPH::Float3 &inFrom, const JPH::Float3 &inTo, JPH::ColorArg inColor ) override;

	void DrawTriangle( JPH::Vec3Arg inV1, JPH::Vec3Arg inV2, JPH::Vec3Arg inV3, JPH::ColorArg inColor ) override;

	Batch CreateTriangleBatch( const Triangle *inTriangles, int inTriangleCount ) override;
	Batch CreateTriangleBatch( const Vertex *inVertices, int inVertexCount, const uint32 *inIndices, int inIndexCount ) override;

	// This parameter list sucks
	void DrawGeometry( JPH::Mat44Arg inModelMatrix, const JPH::AABox &inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef &inGeometry, ECullMode inCullMode = ECullMode::CullBackFace, ECastShadow inCastShadow = ECastShadow::On, EDrawMode inDrawMode = EDrawMode::Solid ) override;

	void DrawText3D( JPH::Vec3Arg inPosition, const std::string &inString, JPH::ColorArg inColor = JPH::Color::sWhite, float inHeight = 0.5f ) override;

private:
	void DrawTriangle_Internal( const JPH::Float3 &v1, const JPH::Float3 &v2, const JPH::Float3 &v3, JPH::Color color );

	//GLuint m_vao, m_vbo, m_ebo;

};

MyDebugRenderer::MyDebugRenderer()
{
#if 0
	glGenVertexArrays( 1, &m_vao );
	glGenBuffers( 1, &m_vbo );
	glGenBuffers( 1, &m_ebo );

	glBindVertexArray( m_vao );
	glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_ebo );

	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );
	glEnableVertexAttribArray( 2 );
	glEnableVertexAttribArray( 3 );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void *)( offsetof( Vertex, mPosition ) ) );
	glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void *)( offsetof( Vertex, mNormal ) ) );
	glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void *)( offsetof( Vertex, mUV ) ) );
	glVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( Vertex ), (void *)( offsetof( Vertex, mColor ) ) );
#endif

	DebugRenderer::Initialize();
}

MyDebugRenderer::~MyDebugRenderer()
{
#if 0
	glDeleteBuffers( 1, &m_ebo );
	glDeleteBuffers( 1, &m_vbo );
	glDeleteVertexArrays( 1, &m_vao );
#endif
}

void MyDebugRenderer::DrawLine( const JPH::Float3 &inFrom, const JPH::Float3 &inTo, JPH::ColorArg inColor )
{
	vec3_t v1, v2;
	VectorCopy( (const float *)&inFrom, v1 );
	VectorCopy( (const float *)&inTo, v2 );

	v1[0] *= JOLT2QUAKE_FACTOR;
	v1[1] *= JOLT2QUAKE_FACTOR;
	v1[2] *= JOLT2QUAKE_FACTOR;
	v2[0] *= JOLT2QUAKE_FACTOR;
	v2[1] *= JOLT2QUAKE_FACTOR;
	v2[2] *= JOLT2QUAKE_FACTOR;

	// Just use the R_Draw function
	R_DrawLine( v1, v2, inColor.GetUInt32() );
}

void MyDebugRenderer::DrawTriangle( JPH::Vec3Arg inV1, JPH::Vec3Arg inV2, JPH::Vec3Arg inV3, JPH::ColorArg inColor )
{
	//DrawTriangle_Internal( JPH::Float3( inV1.GetX(), inV1.GetY(), inV1.GetZ() ), JPH::Float3( inV2.GetX(), inV2.GetY(), inV2.GetZ() ), JPH::Float3( inV3.GetX(), inV3.GetY(), inV3.GetZ() ), inColor );
}

MyDebugRenderer::Batch MyDebugRenderer::CreateTriangleBatch( const Triangle *inTriangles, int inTriangleCount )
{
#if 0
	glBindVertexArray( m_vao );
	glBindBuffer( GL_ARRAY_BUFFER, m_vbo );

	glBufferData( GL_ARRAY_BUFFER, inTriangleCount * sizeof( Triangle ), inTriangles, GL_STREAM_DRAW );

	GL_UseProgram( glProgs.joltProg );

	DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixIdentity();

	DirectX::XMFLOAT4X4A modelMatrixStore;
	DirectX::XMStoreFloat4x4A( &modelMatrixStore, modelMatrix );

	glUniformMatrix4fv( 4, 1, GL_FALSE, (const GLfloat *)&modelMatrixStore );
	glUniformMatrix4fv( 5, 1, GL_FALSE, (const GLfloat *)&tr.viewMatrix );
	glUniformMatrix4fv( 6, 1, GL_FALSE, (const GLfloat *)&tr.projMatrix );

	glDrawArrays( GL_TRIANGLES, 0, inTriangleCount / 3 );
#endif

	return nullptr;
}

MyDebugRenderer::Batch MyDebugRenderer::CreateTriangleBatch( const Vertex *inVertices, int inVertexCount, const uint32 *inIndices, int inIndexCount )
{
#if 0
	glBindVertexArray( m_vao );
	glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_ebo );

	glBufferData( GL_ARRAY_BUFFER, inVertexCount * sizeof( Vertex ), inVertices, GL_STREAM_DRAW );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, inIndexCount * sizeof( uint32 ), inIndices, GL_STREAM_DRAW );

	GL_UseProgram( glProgs.joltProg );

	DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixIdentity();

	DirectX::XMFLOAT4X4A modelMatrixStore;
	DirectX::XMStoreFloat4x4A( &modelMatrixStore, modelMatrix );

	glUniformMatrix4fv( 4, 1, GL_FALSE, (const GLfloat *)&modelMatrixStore );
	glUniformMatrix4fv( 5, 1, GL_FALSE, (const GLfloat *)&tr.viewMatrix );
	glUniformMatrix4fv( 6, 1, GL_FALSE, (const GLfloat *)&tr.projMatrix );

	glDrawElements( GL_TRIANGLES, inIndexCount, GL_UNSIGNED_INT, nullptr );
#endif

	return nullptr;
}

void MyDebugRenderer::DrawGeometry( JPH::Mat44Arg inModelMatrix, const JPH::AABox &inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef &inGeometry, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode )
{

}

void MyDebugRenderer::DrawText3D( JPH::Vec3Arg inPosition, const std::string &inString, JPH::ColorArg inColor, float inHeight )
{

}

//=================================================================================================

void MyDebugRenderer::DrawTriangle_Internal( const JPH::Float3 &v1, const JPH::Float3 &v2, const JPH::Float3 &v3, JPH::Color color )
{
	//R_DrawPoly( (const float *)&v1, (const float *)&v2, (const float *)&v3, color.GetUInt32() );
}

//=================================================================================================

#endif

class QuakeStreamOut final : public JPH::StreamOut
{
private:
	fsHandle_t m_handle;

public:
	QuakeStreamOut( const char *filename )
	{
		m_handle = FileSystem::OpenFileWrite( filename, FS_GAMEDIR );
		assert( m_handle );
	}

	~QuakeStreamOut() override
	{
		assert( m_handle );
		FileSystem::CloseFile( m_handle );
	}

	void WriteBytes( const void *inData, size_t inNumBytes ) override
	{
		FileSystem::WriteFile( inData, inNumBytes, m_handle );
	}

	bool IsFailed() const override
	{
		return false;
	}

};

static QuakeStreamOut *s_streamOut;
static MyDebugRenderer *s_debugRenderer;
//static JPH::DebugRendererRecorder *s_debugRendererRecorder;

void R_JoltInitRenderer()
{
	s_debugRenderer = new MyDebugRenderer();
	s_streamOut = new QuakeStreamOut( "jolt.bin" );
	//s_debugRendererRecorder = new JPH::DebugRendererRecorder( *s_streamOut );
}

void R_JoltShutdownRenderer()
{
	//delete s_debugRendererRecorder;
	delete s_streamOut;
	delete s_debugRenderer;
}

void R_JoltDrawBodies()
{
	if ( !r_drawJolt.GetBool() )
	{
		return;
	}

	JPH::BodyManager::DrawSettings drawSettings
	{
		.mDrawGetSupportFunction = true,						///< Draw the GetSupport() function, used for convex collision detection	
		.mDrawSupportDirection = true,							///< When drawing the support function, also draw which direction mapped to a specific support point
		.mDrawGetSupportingFace = true,						///< Draw the faces that were found colliding during collision detection
		.mDrawShape = true,										///< Draw the shapes of all bodies
		.mDrawShapeWireframe = true,							///< When mDrawShape is true and this is true, the shapes will be drawn in wireframe instead of solid.
		//.mDrawShapeColor = JPH::EShapeColor::MotionTypeColor,	///< Coloring scheme to use for shapes
		.mDrawBoundingBox = true,								///< Draw a bounding box per body
		.mDrawCenterOfMassTransform = true,						///< Draw the center of mass for each body
		.mDrawWorldTransform = true,							///< Draw the world transform (which can be different than the center of mass) for each body
		.mDrawVelocity = true,									///< Draw the velocity vector for each body
		.mDrawMassAndInertia = true,							///< Draw the mass and inertia (as the box equivalent) for each body
		.mDrawSleepStats = true,								///< Draw stats regarding the sleeping algorithm of each body
		.mDrawNames = false										///< (Debug only) Draw the object names for each body
	};

	JPH::PhysicsSystem *physicsSystem = Physics::GetPhysicsSystem();

	physicsSystem->DrawBodies( drawSettings, s_debugRenderer );
#if 0
	physicsSystem->DrawBodies( drawSettings, s_debugRendererRecorder );
	s_debugRendererRecorder->EndFrame();
#endif
}
 
#endif // !NO_JOLT_DEBUG
