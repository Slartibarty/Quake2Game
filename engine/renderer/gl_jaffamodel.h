
#pragma once

struct renderJaffaMesh_t
{
	uint32			offset, count;
	material_t *	material;
};

struct renderJaffaBone_t
{
	vec3 xyz;
	vec3 rot;		// Quaternion
};

struct renderJaffaAnimation_t
{

};

struct renderJaffaModel_t
{
	GLuint					vao, vbo, ebo;
	GLenum					type;			// index type

	uint32					numMeshes;
	renderJaffaMesh_t *		pMeshes;
	uint32					numBones;
	renderJaffaBone_t *		pBones;
};

void Mod_LoadJaffaModel( model_t *pMod, const void *pBuffer, [[maybe_unused]] int bufferLength );

void R_DrawJaffaModel( entity_t *e );
