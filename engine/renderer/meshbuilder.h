
#pragma once

#include "gl_local.h"

#include <vector>
#include <limits>

/*
===================================================================================================

	A mesh builder specialised for building GUI mesh data

===================================================================================================
*/

struct guiVertex_t
{
	float x, y;
	float s, t;
	uint32 rgba;

	void Position( float X, float Y )
	{
		x = X;
		y = Y;
	}

	void TexCoord( float S, float T )
	{
		s = S;
		t = T;
	}

	void Color( uint32 color )
	{
		rgba = color;
	}
};

// gui elements are drawn clockwise
struct guiRect_t
{
	/*
	   1>     2v
		x    x


		x    x
	   ^4     <3
	*/
	guiVertex_t v1;
	guiVertex_t v2;
	guiVertex_t v3;
	guiVertex_t v4;
};

template< typename index_t >
class GUIMeshBuilder_Base
{
public:
	GUIMeshBuilder_Base()
	{
		// reserve 128 quads
		m_elements.reserve( 128 );
		m_indices.reserve( 128 );
	}

	void AddElement( guiRect_t &rect )
	{
		m_elements.push_back( rect );

		indexElement_t &index = m_indices.emplace_back();
		
		index[0] = 0 + m_baseIndex;
		index[1] = 1 + m_baseIndex;
		index[2] = 2 + m_baseIndex;
		index[3] = m_restartIndex;

		index[4] = 0 + m_baseIndex;
		index[5] = 2 + m_baseIndex;
		index[6] = 3 + m_baseIndex;
		index[7] = m_restartIndex;

		m_baseIndex += 4;
	}

	// clear stored data
	void Reset()
	{
		m_elements.clear();
		m_indices.clear();
		m_baseIndex = 0;
	}

	float *GetVertexArray()
	{
		return (float *)m_elements.data();
	}

	index_t *GetIndexArray()
	{
		return (index_t *)m_indices.data();
	}

	size_t GetVertexArraySize()
	{
		return m_elements.size() * sizeof( guiRect_t );
	}

	size_t GetIndexArraySize()
	{
		return m_indices.size() * sizeof( indexElement_t );
	}

	size_t GetIndexArrayCount()
	{
		return m_indices.size() * ( sizeof( indexElement_t ) / sizeof( index_t ) );
	}

	uint32 GetBaseIndex()
	{
		return m_baseIndex;
	}

	bool HasData()
	{
		return m_baseIndex != 0;
	}

private:

	static constexpr index_t m_restartIndex = std::numeric_limits<index_t>::max();

	// Hack because std::vectors can't contain raw arrays
	struct indexElement_t
	{
		index_t arr[8];

		index_t &operator[] ( size_t i )
		{
			return arr[i];
		}
	};

	uint32 m_baseIndex = 0;

	std::vector<guiRect_t> m_elements;
	std::vector<indexElement_t> m_indices;

};

using GUIMeshBuilder = GUIMeshBuilder_Base< uint16 >;

/*
===================================================================================================

	A mesh builder for drawing debug meshes

===================================================================================================
*/

struct debugVertex_t
{
	float x, y, z;

	void Position( float X, float Y, float Z )
	{
		x = X;
		y = Y;
		z = Z;
	}
};

template< typename index_t >
class DebugMeshBuilder
{
public:
	DebugMeshBuilder()
	{
		m_vertices.reserve( 128 );
	}

	void AddVertex( guiRect_t &rect )
	{

	}

	// clear stored data
	void Reset()
	{
		m_vertices.clear();
	}

	float *GetVertexArray()
	{
		return (float *)m_vertices.data();
	}

	size_t GetVertexArraySize()
	{
		return m_vertices.size() * sizeof( debugVertex_t );
	}

	bool HasData()
	{
		return m_vertices.size() != 0;
	}

private:

	std::vector<debugVertex_t> m_vertices;

};
