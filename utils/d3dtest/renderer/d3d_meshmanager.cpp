
#include "d3d_local.h"

#include <vector>

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"

namespace Renderer
{

// Renderer representation of a thing in a scene
struct model_t
{
	char name[MAX_ASSETPATH];

	ID3D11Buffer *vertexBuffer;
	ID3D11Buffer *indexBuffer;
	uint32 numVertices;
	uint32 numIndices;
};

static struct modelManager_t
{
	std::vector<model_t> models;
} modelMan;

static ID3D11Buffer *CreateVertexBuffer( void *vertices, uint32 numVertices )
{
	const D3D11_BUFFER_DESC desc{
		.ByteWidth = numVertices * sizeof( meshVertex_t ),
		.Usage = D3D11_USAGE_IMMUTABLE,
		.BindFlags = D3D11_BIND_VERTEX_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	};

	const D3D11_SUBRESOURCE_DATA initialData{
		.pSysMem = vertices,
		.SysMemPitch = 0,
		.SysMemSlicePitch = 0
	};

	ID3D11Buffer *buffer;

	HRESULT hr = d3d.device->CreateBuffer( &desc, &initialData, &buffer );
	assert( SUCCEEDED( hr ) );

	return buffer;
}

static ID3D11Buffer *CreateIndexBuffer( void *indices, uint32 numIndices )
{
	const D3D11_BUFFER_DESC desc{
		.ByteWidth = numIndices * sizeof( meshIndex_t ),
		.Usage = D3D11_USAGE_IMMUTABLE,
		.BindFlags = D3D11_BIND_INDEX_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	};

	const D3D11_SUBRESOURCE_DATA initialData{
		.pSysMem = indices,
		.SysMemPitch = 0,
		.SysMemSlicePitch = 0
	};

	ID3D11Buffer *buffer;

	HRESULT hr = d3d.device->CreateBuffer( &desc, &initialData, &buffer );
	assert( SUCCEEDED( hr ) );

	return buffer;
}

struct objContext_t
{
	char *buffer;
	size_t bufferLength;
};

static void LoadOBJ_Callback( void *ctx, const char *filename, int is_mtl, const char *obj_filename, char **buf, size_t *len )
{
	objContext_t *context = (objContext_t *)ctx;

	*buf = context->buffer;
	*len = context->bufferLength;
}

static void LoadOBJ( model_t &model, void *buffer, fsSize_t bufferLength )
{
	tinyobj_attrib_t attrib;
	tinyobj_shape_t *shapes;
	size_t numShapes;
	tinyobj_material_t *materials;
	size_t numMaterials;

	objContext_t context{ (char *)buffer, (size_t)bufferLength };

	int result = tinyobj_parse_obj( &attrib, &shapes, &numShapes,
		&materials, &numMaterials, nullptr, LoadOBJ_Callback, (void *)&context, TINYOBJ_FLAG_TRIANGULATE );
	assert( result == TINYOBJ_SUCCESS );

	meshVertex_t *flatVertices = (meshVertex_t *)Mem_Alloc( attrib.num_vertices * sizeof( meshVertex_t ) );
	for ( uint32 i = 0; i < attrib.num_vertices; ++i )
	{
		flatVertices[i].pos.SetFromLegacy( attrib.vertices + ( i * 3 ) );
		flatVertices[i].tex.Set( 1.0f, 1.0f );
		flatVertices[i].nor.Set( 0.0f, 0.0f, 0.0f );
	}

	uint32 *flatIndices = (uint32 *)Mem_Alloc( attrib.num_faces * sizeof( uint32 ) );
	for ( uint32 i = 0; i < attrib.num_faces; ++i )
	{
		flatIndices[i] = attrib.faces[i].v_idx;
	}

	// Now configure our model

	model.vertexBuffer = CreateVertexBuffer( flatVertices, attrib.num_vertices );
	model.indexBuffer = CreateIndexBuffer( flatIndices, attrib.num_faces );
	model.numVertices = attrib.num_vertices;
	model.numIndices = attrib.num_faces;

	// Done

	Mem_Free( flatIndices );
}

modelIndex_t ModelForName( const char *filename, void *buffer, fsSize_t bufferLength )
{
	const modelIndex_t numModels = static_cast<modelIndex_t>( modelMan.models.size() );
	for ( modelIndex_t modelIndex = 0; modelIndex < numModels; ++modelIndex )
	{
		const model_t &model = modelMan.models[modelIndex];

		if ( Q_strcmp( model.name, filename ) == 0 )
		{
			// Already loaded
			return modelIndex;
		}
	}

	// Not loaded, so load it

	if ( !buffer || bufferLength == 0 )
	{
		// Yeah
		Com_FatalError( "Hey, tried to get a model with no buffer!\n" );
	}

	model_t &model = modelMan.models.emplace_back();

	Q_strcpy_s( model.name, filename );

	LoadOBJ( model, buffer, bufferLength );
}

} // namespace Renderer
