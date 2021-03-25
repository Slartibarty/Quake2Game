
#pragma once

#include "gl_local.h"

#include <vector>

/*
===============================================================================

	A mesh builder specialised for building GUI mesh data

===============================================================================
*/

struct guiVertex_t
{
	float x, y;
	float s, t;
};

// gui elements are drawn clockwise
struct guiRectangle_t
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
class qGUIMeshBuilder_Base
{
public:
	qGUIMeshBuilder_Base()
	{
		// reserve 128 quads
		m_elements.reserve( 128 );
		m_indices.reserve( 128 );
	}

	void AddElement( guiRectangle_t &rect )
	{
		m_elements.push_back( rect );

		indexElement_t &index = m_indices.emplace_back();
		
		index[0] = 0 + m_baseIndex;
		index[1] = 1 + m_baseIndex;
		index[2] = 2 + m_baseIndex;
		index[3] = USHRT_MAX;

		index[4] = 0 + m_baseIndex;
		index[5] = 2 + m_baseIndex;
		index[6] = 3 + m_baseIndex;
		index[7] = USHRT_MAX;

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
		return m_elements.size() * sizeof( guiRectangle_t );
	}

	size_t GetIndexArraySize()
	{
		return m_indices.size() * sizeof( indexElement_t );
	}

	size_t GetIndexArrayCount()
	{
		return m_indices.size() * ( sizeof( indexElement_t ) / sizeof( index_t ) );
	}

	bool HasData()
	{
		return m_baseIndex != 0;
	}

private:

	// Hack because std::vectors can't contain raw arrays
	struct indexElement_t
	{
		index_t arr[8];

		index_t &operator[] ( size_t i )
		{
			return arr[i];
		}
	};

	uint m_baseIndex = 0;

	std::vector<guiRectangle_t> m_elements;
	std::vector<indexElement_t> m_indices;

};

using qGUIMeshBuilder = qGUIMeshBuilder_Base< uint16 >;
