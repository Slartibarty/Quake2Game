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

/*
===========================================================
	LineBuilder

	Builds discrete lines, equivalent to GL_LINES.
	It is up to the user to reserve lines to prevent
	lots of reallocations.
===========================================================
*/
class LineBuilder
{
private:
	struct lineVertex_t
	{
		vec3 pos;
		uint32 col;
	};

	struct lineSegment_t
	{
		lineVertex_t v1;
		lineVertex_t v2;
	};

	std::vector<lineSegment_t> m_segments;

public:
	LineBuilder() = default;
	~LineBuilder() = default;

	// Make a space reservation
	void Reserve( size_t count )
	{
		m_segments.reserve( count );
	}

	void AddLine( const vec3 &start, uint32 startCol, const vec3 &end, uint32 endCol )
	{
		m_segments.push_back( { {start, startCol}, {end, endCol} } );
	}

};

/*
===========================================================
	Helper Functions

	Misc helpers for drawing primitives
===========================================================
*/

void LineBuilder_DrawWorldAxes( LineBuilder &bld );
void LineBuilder_DrawCrosshair( LineBuilder &bld, int windowWidth, int windowHeight );
void LineBuilder_DrawFrameRect( LineBuilder &bld, int windowWidth, int windowHeight );
