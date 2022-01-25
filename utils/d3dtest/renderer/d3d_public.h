
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

	using modelIndex_t = uint32;

	struct meshVertex_t
	{
		vec3 pos;
		vec2 tex;
		vec3 nor;
	};

	using meshIndex_t = uint32;

	// Gets a model index by name
	modelIndex_t ModelForName( const char *filename, void *buffer = nullptr, fsSize_t bufferLength = 0 );

}
