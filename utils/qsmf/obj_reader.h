//=================================================================================================
// Think Tank 2
// Copyright (c) Slartibarty. All Rights Reserved.
//=================================================================================================

#pragma once

#include "../../core/sys_types.h"

#include <vector>

struct float2
{
	float x, y;
};

struct float3
{
	float x, y, z;
};

struct uint3
{
	uint16 x, y, z;
};

struct vertex_t
{
	float3 vertex;
	float2 texcoord;
	float3 normal;
};

struct index_t
{
	uint16 ivertex, itexcoord, inormal;
};

struct index3_t
{
	index_t a, b, c;
};

class OBJReader
{
private:
	static constexpr auto Delimiters = " \n\r\t";
	static constexpr auto SubDelimiters = "/";

	std::vector<vertex_t> m_vertices;
	std::vector<uint3> m_indices;

	// Takes a null terminated buffer
	void Parse( char *buffer );

public:
	// Constructor: Try to parse an OBJ from a buffer
	OBJReader( char *buffer )
	{
		Parse( buffer );
	}

	float3 *GetVertices()
	{
		return (float3 *)m_vertices.data();
	}

	size_t GetNumVertices()
	{
		return m_vertices.size();
	}

	size_t GetVerticesSize()
	{
		return m_vertices.size() * sizeof( vertex_t );
	}

	uint16 *GetIndices()
	{
		return (uint16 *)m_indices.data();
	}

	size_t GetNumIndices()
	{
		return m_indices.size() * 3;
	}

	size_t GetIndicesSize()
	{
		return m_indices.size() * sizeof( uint3 );
	}

};
