
#include "engine.h"

#include <vector>

#include "renderer/tinyobj_loader_c.h"

#include "renderer/d3d_public.h"

namespace ObjectManager
{

// Client representation of a thing in a scene
struct object_t
{
	char name[MAX_ASSETPATH];

	Renderer::modelIndex_t modelIndex;
};

static struct objectLocal_t
{
	std::vector<object_t> objects;
} objMan;

objectIndex_t ObjectForName( const char *filename )
{
	const objectIndex_t numObjects = static_cast<objectIndex_t>( objMan.objects.size() );
	for ( objectIndex_t objectIndex = 0; objectIndex < numObjects; ++objectIndex )
	{
		const object_t &object = objMan.objects[objectIndex];

		if ( Q_strcmp( object.name, filename ) == 0 )
		{
			// Already loaded
			return objectIndex;
		}
	}

	void *meshBuffer;
	fsSize_t meshBufferLength = FileSystem::LoadFile( "models/scene.obj", &meshBuffer );
	if ( !meshBuffer )
	{
		Com_FatalError( "DUDE!\n" );
	}

	object_t &object = objMan.objects.emplace_back();

	// Feed it into the physics system as a body
	PhysicsSystem::AddBodyForOBJ( meshBuffer, meshBufferLength );

	// Hand it off to the renderer
	Renderer::ModelForName( filename, meshBuffer, meshBufferLength );

	FileSystem::FreeFile( meshBuffer );

	return static_cast<objectIndex_t>( objMan.objects.size() - 1 );
}

} // namespace ObjectManager
