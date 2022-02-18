
#include "gl_local.h"

#include "iqm.h"

struct mIQMVertex_t
{
	float position[3];
	float texcoord[2];
	float normal[3];
	float tangent[4];
	uint8 blendIndex[4];
	uint8 blendWeight[4];
};

struct mIQMMesh_t
{
	material_t *pMaterial;
	uint numIndices, indexOffset;
};

struct mIQM_t
{
	std::vector<mIQMMesh_t> meshes;
	GLuint vao, vbo, ebo, ubo;
};

// Returns an interleaved vertex buffer
static void IQM_UploadVertices( const iqmheader *hdr )
{
	const byte *buffer = (const byte *)hdr;

	const float *inPosition = nullptr;
	const float *inTexcoord = nullptr;
	const float *inNormal = nullptr;
	const float *inTangent = nullptr;
	const byte *inBlendIndex = nullptr;
	const byte *inBlendWeight = nullptr;

	const iqmvertexarray *beginVertexArray = (const iqmvertexarray *)( buffer + hdr->ofs_vertexarrays );
	const iqmvertexarray *endVertexArray = beginVertexArray + hdr->num_vertexarrays;

	for ( const iqmvertexarray *va = beginVertexArray; va < endVertexArray; ++va )
	{
		switch ( va->type )
		{
		case IQM_POSITION:
			inPosition = (const float *)( buffer + va->offset );
			break;
		case IQM_TEXCOORD:
			inTexcoord = (const float *)( buffer + va->offset );
			break;
		case IQM_NORMAL:
			inNormal = (const float *)( buffer + va->offset );
			break;
		case IQM_TANGENT:
			inTangent = (const float *)( buffer + va->offset );
			break;
		case IQM_BLENDINDEXES:
			inBlendIndex = (const byte *)( buffer + va->offset );
			break;
		case IQM_BLENDWEIGHTS:
			inBlendWeight = (const byte *)( buffer + va->offset );
			break;
		}
	}

	assert( inPosition && inTexcoord && inNormal && inTangent && inBlendIndex && inBlendWeight );

	/*if ( !( inPosition && inTexcoord && inNormal && inTangent && inBlendIndex && inBlendWeight ) )
	{
		Com_FatalError( "IQM file was missing a vertex channel" );
	}*/

	const uint numVertices = hdr->num_vertexes;

	mIQMVertex_t *vertices = (mIQMVertex_t *)Mem_ClearedAlloc( numVertices * sizeof( mIQMVertex_t ) );
	
	for ( uint i = 0; i < numVertices; ++i )
	{
		mIQMVertex_t &v = vertices[i];
		if ( inPosition )		{ memcpy( v.position, inPosition + ( i * 3 ), sizeof( v.position ) ); }
		if ( inTexcoord )		{ memcpy( v.texcoord, inTexcoord + ( i * 2 ), sizeof( v.texcoord ) ); }
		if ( inNormal )			{ memcpy( v.normal, inNormal + ( i * 3 ), sizeof( v.normal ) ); }
		if ( inTangent )		{ memcpy( v.tangent, inTangent + ( i * 4 ), sizeof( v.tangent ) ); }
		if ( inBlendIndex )		{ memcpy( v.blendIndex, inBlendIndex + ( i * 4 ), sizeof( v.blendIndex ) ); }
		if ( inBlendWeight )	{ memcpy( v.blendWeight, inBlendWeight + ( i * 4 ), sizeof( v.blendWeight ) ); }
	}

	glBufferData( GL_ARRAY_BUFFER, numVertices * sizeof( mIQMVertex_t ), vertices, GL_STATIC_DRAW );

	Mem_Free( vertices );
}

static void IQM_UploadIndices( const iqmheader *hdr )
{
	const byte *buffer = (const byte *)hdr;

	iqmtriangle *tris = (iqmtriangle *)( buffer + hdr->ofs_triangles );

	glBufferData( GL_ELEMENT_ARRAY_BUFFER, hdr->num_triangles * sizeof( iqmtriangle ), tris, GL_STATIC_DRAW );
}

static void IQM_LoadMeshes( const iqmheader *hdr, mIQM_t *iqm )
{
	if ( hdr->num_meshes == 0 )
	{
		return;
	}

	IQM_UploadVertices( hdr );
	IQM_UploadIndices( hdr );

	mIQMMesh_t &iqmMesh = iqm->meshes.emplace_back();

	iqmMesh.pMaterial = nullptr;
	iqmMesh.numIndices = hdr->num_triangles * 3;
	iqmMesh.indexOffset = 0;

	//const void *buffer = (const void *)hdr;
}

void Mod_LoadIQM( model_t *mod, const void *buffer, fsSize_t bufferLength )
{
	if ( bufferLength <= sizeof( iqmheader ) )
	{
		Com_Error( "IQM model was corrupt" );
	}

	const iqmheader *hdr = (const iqmheader *)buffer;

	// Redundant because we check this earlier, but keeping here for completion
	if ( memcmp( hdr->magic, IQM_MAGIC, sizeof( IQM_MAGIC ) ) != 0 )
	{
		Com_Error( "IQM magic was not " IQM_MAGIC );
	}

	if ( hdr->version != IQM_VERSION )
	{
		Com_Error( "IQM version was not " STRINGIFY( IQM_VERSION ) );
	}

	mIQM_t *iqm = (mIQM_t *)Hunk_Alloc( sizeof( mIQM_t ) );

	// Generate all buffers
	glGenVertexArrays( 1, &iqm->vao );
	glGenBuffers( 3, &iqm->vbo );

	glBindVertexArray( iqm->vao );
	glBindBuffer( GL_ARRAY_BUFFER, iqm->vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, iqm->ebo );
	glBindBuffer( GL_UNIFORM_BUFFER, iqm->ubo );

	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );
	glEnableVertexAttribArray( 2 );
	glEnableVertexAttribArray( 3 );
	glEnableVertexAttribArray( 4 );
	glEnableVertexAttribArray( 5 );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( mIQMVertex_t ), (void *)( offsetof( mIQMVertex_t, position ) ) );
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( mIQMVertex_t ), (void *)( offsetof( mIQMVertex_t, texcoord ) ) );
	glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof( mIQMVertex_t ), (void *)( offsetof( mIQMVertex_t, normal ) ) );
	glVertexAttribPointer( 3, 4, GL_FLOAT, GL_FALSE, sizeof( mIQMVertex_t ), (void *)( offsetof( mIQMVertex_t, tangent ) ) );
	glVertexAttribPointer( 4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( mIQMVertex_t ), (void *)( offsetof( mIQMVertex_t, blendIndex ) ) );
	glVertexAttribPointer( 5, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( mIQMVertex_t ), (void *)( offsetof( mIQMVertex_t, blendWeight ) ) );

	IQM_LoadMeshes( hdr, iqm );

	mod->type = mod_iqm;

	mod->mins[0] = -32.0f;
	mod->mins[1] = -32.0f;
	mod->mins[2] = -32.0f;
	mod->maxs[0] = 32.0f;
	mod->maxs[1] = 32.0f;
	mod->maxs[2] = 32.0f;
}

void R_DrawIQM( entity_t *e )
{
	using namespace DirectX;

	XMMATRIX modelMatrix = XMMatrixMultiply(
		XMMatrixRotationX( DEG2RAD( e->angles[ROLL] ) ) * XMMatrixRotationY( DEG2RAD( e->angles[PITCH] ) ) * XMMatrixRotationZ( DEG2RAD( e->angles[YAW] ) ),
		XMMatrixTranslation( e->origin[0], e->origin[1], e->origin[2] )
	);

	XMFLOAT4X4A modelMatrixStore;
	XMStoreFloat4x4A( &modelMatrixStore, modelMatrix );

	GL_UseProgram( glProgs.iqmProg );

	glUniformMatrix4fv( 4, 1, GL_FALSE, (const GLfloat *)&modelMatrixStore );
	glUniformMatrix4fv( 5, 1, GL_FALSE, (const GLfloat *)&tr.viewMatrix );
	glUniformMatrix4fv( 6, 1, GL_FALSE, (const GLfloat *)&tr.projMatrix );

	const mIQM_t *iqm = (mIQM_t * )e->model->extradata;

	glBindVertexArray( iqm->vao );
	glBindBuffer( GL_ARRAY_BUFFER, iqm->vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, iqm->ebo );

	glDrawElements( GL_TRIANGLES, iqm->meshes[0].numIndices, GL_UNSIGNED_INT, (void *)( 0 ) );

	/*for ( uint i = 0; i < (uint)iqm->meshes.size(); ++i )
	{
		glDrawElements( GL_TRIANGLES, iqm->meshes[i].numIndices, GL_UNSIGNED_INT, (void *)( (uintptr_t)( iqm->meshes[i].numIndices / sizeof( uint32 ) ) ) );
	}*/
}
