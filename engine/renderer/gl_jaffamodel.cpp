
#include "gl_local.h"

/*
=======================================
	Loading
=======================================
*/

void Mod_LoadJaffaModel( model_t *pMod, const void *pBuffer, int bufferLength )
{
	using namespace fmtJMDL;

	if ( bufferLength <= sizeof( header_t ) )
	{
		Com_Error( S_COLOR_RED "Error: JaffaModel was fucked up" );
	}

	const header_t *pHeader = (const header_t *)pBuffer;

	if ( pHeader->version != version )
	{
		Com_Error( S_COLOR_RED "Error: JaffaModel version was fucked up" );
	}

	renderJaffaModel_t *pJaffaModel = new renderJaffaModel_t;

	const mesh_t *meshes = reinterpret_cast<const mesh_t *>( (const byte *)pBuffer + pHeader->offsetMeshes );

	pJaffaModel->pMeshes = new renderJaffaMesh_t[pHeader->numMeshes];
	pJaffaModel->numMeshes = pHeader->numMeshes;
	for ( uint32 i = 0; i < pHeader->numMeshes; ++i )
	{
		pJaffaModel->pMeshes[i].offset = meshes[i].offsetIndices;
		pJaffaModel->pMeshes[i].count = meshes[i].countIndices;
		pJaffaModel->pMeshes[i].material = GL_FindMaterial( meshes[i].materialName );
	}

	pJaffaModel->type = ( pHeader->flags & eBigIndices ) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;

	glGenVertexArrays( 1, &pJaffaModel->vao );
	glGenBuffers( 2, &pJaffaModel->vbo );			// Gen the vbo and ebo in one go

	glBindVertexArray( pJaffaModel->vao );
	glBindBuffer( GL_ARRAY_BUFFER, pJaffaModel->vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pJaffaModel->ebo );

	// Determine vertex attributes

	const bool usesBones = pHeader->flags & eUsesBones;
	const size_t structureSize = usesBones ? sizeof( staticVertex_t ) : sizeof( skinnedVertex_t );

	// xyz, st, normal, tangent
	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );
	glEnableVertexAttribArray( 2 );
	glEnableVertexAttribArray( 3 );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, structureSize, (void *)( 0 ) );
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, structureSize, (void *)( 3 * sizeof( GLfloat ) ) );
	glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, structureSize, (void *)( 5 * sizeof( GLfloat ) ) );
	glVertexAttribPointer( 3, 3, GL_FLOAT, GL_FALSE, structureSize, (void *)( 8 * sizeof( GLfloat ) ) );

	if ( usesBones )
	{
		// bone indices, weights
		glEnableVertexAttribArray( 4 );
		glEnableVertexAttribArray( 5 );

		glVertexAttribPointer( 4, 4, GL_UNSIGNED_BYTE, GL_FALSE, structureSize, (void *)( 11 * sizeof( GLfloat ) ) );
		glVertexAttribPointer( 5, 4, GL_UNSIGNED_BYTE, GL_FALSE, structureSize, (void *)( 11 * sizeof( GLfloat ) + sizeof( uint8 ) * maxVertexWeights ) );
	}



}

/*
=======================================
	Drawing
=======================================
*/

void R_DrawJaffaModel( entity_t *e )
{

}
