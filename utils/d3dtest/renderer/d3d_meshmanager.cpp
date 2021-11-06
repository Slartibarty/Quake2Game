
#include "d3d_local.h"

#include <vector>

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"

namespace Renderer
{

struct objContext_t
{
	char *buffer;
	size_t bufferLength;
};

meshManager_t meshMan;

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

meshID_t AddMesh( void *vertices, uint32 numVertices, void *indices, uint32 numIndices )
{
	mesh_t &mesh = meshMan.meshes.emplace_back();

	mesh.vertexBuffer = CreateVertexBuffer( vertices, numVertices );
	mesh.indexBuffer = CreateIndexBuffer( indices, numIndices );
	mesh.numVertices = numVertices;
	mesh.numIndices = numIndices;

	return 0;
}

static void LoadOBJ_Callback( void *ctx, const char *filename, int is_mtl, const char *obj_filename, char **buf, size_t *len )
{
	objContext_t *context = (objContext_t *)ctx;

	*buf = context->buffer;
	*len = context->bufferLength;
}

void LoadOBJ( void *buffer, fsSize_t bufferLength )
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

	AddMesh( flatVertices, attrib.num_vertices, flatIndices, attrib.num_faces );

	Mem_Free( flatIndices );
}

} // namespace Renderer
