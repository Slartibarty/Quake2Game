
#pragma once

namespace Renderer
{
	/*=================================================================================================
		Init
	=================================================================================================*/

	void Init();
	void Shutdown();

	/*=================================================================================================
		Frame
	=================================================================================================*/

	void Frame();

	/*=================================================================================================
		MeshManager
	=================================================================================================*/

	using meshID_t = uint32;

	struct meshVertex_t
	{
		vec3 pos;
		vec2 tex;
		vec3 nor;
	};

	using meshIndex_t = uint32;

	// Adds a render mesh
	meshID_t AddMesh( void *vertices, uint32 numVertices, void *indices, uint32 numIndices );
	void LoadOBJ( void *buffer, fsSize_t bufferLength );

}
