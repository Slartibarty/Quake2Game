
#pragma once

#include "../../core/sys_types.h"

#include "Jolt/Jolt.h"
#include "Jolt/Renderer/DebugRenderer.h"

#include <vector>

#include "local_glm.h"

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

public:
	void BeginFrame( int width, int height );
	void EndFrame( SDL_Window *window );

private:
	void DrawTriangle_Internal( const JPH::Float3 &v1, const JPH::Float3 &v2, const JPH::Float3 &v3, JPH::Color color );

private:
	struct lineVertex_t
	{
		JPH::Float3 pos;
		JPH::Color col;
	};

	struct lineSegment_t
	{
		lineVertex_t v1;
		lineVertex_t v2;
	};

	std::vector<lineSegment_t> m_lineSegments;
	std::vector<Triangle> m_triangles;

	glm::mat4 m_projMatrix;
	glm::mat4 m_viewMatrix;

	GLuint m_triVAO;
	GLuint m_triVBO;
	GLuint m_triEBO;

	GLuint m_lineVAO;
	GLuint m_lineVBO;
};
