/*
===================================================================================================

	Mesh Builders

	The general concept behind these classes is that you gradually build a scene by making calls
	to builder instances, that will then upload and draw all of their content at the end of
	a frame.
	This isn't the most efficient way to do things but it's good enough, and is also API agnostic.

===================================================================================================
*/

#pragma once

#include <vector>

// A simple vertex with position and colour
struct simpleVertex_t
{
	vec3 pos;
	vec3 col;
};

/*
===========================================================
	TriListMeshBuilder

	Blah
===========================================================
*/
class TriListMeshBuilder
{
public:
	TriListMeshBuilder() = default;
	~TriListMeshBuilder() = default;

	size_t GetNumVertices() const
	{
		return m_vertices.size();
	}

	size_t GetSizeInBytes() const
	{
		return m_vertices.size() * sizeof( simpleVertex_t );
	}

	const void *GetData() const
	{
		return reinterpret_cast<const void *>( m_vertices.data() );
	}

	// Make a space reservation
	void Reserve( size_t count )
	{
		m_vertices.reserve( count );
	}

	// Empty the array
	void Clear()
	{
		m_vertices.clear();
	}

	void AddVertex( const simpleVertex_t &vertex )
	{
		m_vertices.push_back( vertex );
	}

private:
	std::vector<simpleVertex_t> m_vertices;

};

/*
===========================================================
	Helper Functions

	Misc helpers for drawing primitives
===========================================================
*/

// Nothing
