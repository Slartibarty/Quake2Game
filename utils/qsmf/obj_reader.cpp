//=================================================================================================
// Think Tank 2
// Copyright (c) Slartibarty. All Rights Reserved.
//=================================================================================================

#include "../../core/core.h"

#include <vector>

#include "obj_reader.h"

#ifdef _WIN32
#define strtok_r strtok_s // jaffaquake doesn't have this
#endif

namespace utl
{
	inline bool IsNewline( char a )
	{
		// lf, cr
		return ( a == '\n' || a == '\r' );
	}
}

#define SMALL_FLOAT 1e-12f

void CalcTriangleTangentSpace( const vec3 &p0, const vec3 &p1, const vec3 &p2,
							   const vec2 &t0, const vec2 &t1, const vec2 &t2,
							   vec3 &sVect/*, vec3 &tVect */ )
{
	// compute the partial derivatives of X, Y, and Z with respect to S and T
	sVect.Zero();
	//tVect.Zero();

	// x, s, t
	vec3 edge01( p1.x - p0.x, t1.x - t0.x, t1.y - t0.y );
	vec3 edge02( p2.x - p0.x, t2.x - t0.x, t2.y - t0.y );

	vec3 cross;
	Vec3CrossProduct( edge01, edge02, cross );
	if ( fabs( cross.x ) > SMALL_FLOAT )
	{
		sVect.x += -cross.y / cross.x;
		//tVect.x += -cross.z / cross.x;
	}

	// y, s, t
	edge01.Set( p1.y - p0.y, t1.x - t0.x, t1.y - t0.y );
	edge02.Set( p2.y - p0.y, t2.x - t0.x, t2.y - t0.y );

	Vec3CrossProduct( edge01, edge02, cross );
	if ( fabs( cross.x ) > SMALL_FLOAT )
	{
		sVect.y += -cross.y / cross.x;
		//tVect.y += -cross.z / cross.x;
	}

	// z, s, t
	edge01.Set( p1.z - p0.z, t1.x - t0.x, t1.y - t0.y );
	edge02.Set( p2.z - p0.z, t2.x - t0.x, t2.y - t0.y );

	Vec3CrossProduct( edge01, edge02, cross );
	if ( fabs( cross.x ) > SMALL_FLOAT )
	{
		sVect.z += -cross.y / cross.x;
		//tVect.z += -cross.z / cross.x;
	}

	// normalize sVect and tVect
	sVect.NormalizeFast();
	//tVect.NormalizeFast();
}

void OBJReader::Parse( char *buffer )
{
	if ( Q_strncmp( buffer, "# Blender ", 10 ) == 0 ) {
		Com_Print( "Blender OBJ detected\n" );
		blenderMode = true;
	}

	std::vector<vec3> rawPositions;		// raw positions
	std::vector<vec2> rawCoords;		// raw texture coordinates
	std::vector<vec3> rawNormals;		// raw vertex normals

	std::vector<index3_t> rawIndices;	// raw indices

	// waste of memory? maybe
	rawPositions.reserve( 2048 );
	rawCoords.reserve( 2048 );
	rawNormals.reserve( 4096 );

	rawIndices.reserve( 4096 );

	// load everything out of the obj

	double start = Time_FloatMilliseconds();

	char *token, *save_ptr;
	token = strtok_r( buffer, Delimiters, &save_ptr );
	for ( ; token && *token != '\0'; token = strtok_r( nullptr, Delimiters, &save_ptr ) )
	{
		vec2 v2;
		vec3 v3;
		index3_t index;

		if ( Q_strcmp( token, "v" ) == 0 )			// position
		{
			// Skip past 'v'
			token = strtok_r( nullptr, Delimiters, &save_ptr );

			v3.x = (float)atof( token );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			v3.y = (float)atof( token );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			v3.z = (float)atof( token );

			rawPositions.push_back( v3 );

			continue;
		}
		if ( Q_strcmp( token, "vt" ) == 0 )			// texture coordinates
		{
			// Vertex normals
			// Skip past 'vt'
			token = strtok_r( nullptr, Delimiters, &save_ptr );

			const char *s1, *s2;
			s1 = token;

#if 1
			v2.x = static_cast<float>( atof( token ) );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			s2 = token;
			v2.y = static_cast<float>( 1.0 - atof( token ) ); // swap for opengl :)
#else
			v2.x = static_cast<float>( atof( token ) );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			s2 = token;
			v2.y = static_cast<float>( atof( token ) ); // swap for opengl :)
#endif
			if ( !blenderMode ) {
				// 3ds max writes an extra 0.0 value
				token = strtok_r( nullptr, Delimiters, &save_ptr );
			}

			//assert( ( v2.x >= 0.0f && v2.x <= 1.0f ) && ( v2.y >= 0.0f && v2.y <= 1.0f ) );

			rawCoords.push_back( v2 );

			continue;
		}
		if ( Q_strcmp( token, "vn" ) == 0 )			// vertex normal
		{
			// Vertex normals
			// Skip past 'vn'
			token = strtok_r( nullptr, Delimiters, &save_ptr );

			v3.x = (float)atof( token );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			v3.y = (float)atof( token );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			v3.z = (float)atof( token );

			rawNormals.push_back( v3 );

			continue;
		}
		if ( Q_strcmp( token, "f" ) == 0 )			// indices
		{
			// Some indices
			// Skip past 'f'
			// Only support triangles: n/n/n
			token = strtok_r( nullptr, Delimiters, &save_ptr );

			auto repeat = [&]( index_t &val )
			{
				char *subtoken, *sub_save_ptr;

				subtoken = strtok_r( token, SubDelimiters, &sub_save_ptr );

				val.iPosition = static_cast<uint32>( atoi( subtoken ) - 1 );
				subtoken = strtok_r( nullptr, SubDelimiters, &sub_save_ptr );
				val.iTexcoord = static_cast<uint32>( atoi( subtoken ) - 1 );
				subtoken = strtok_r( nullptr, SubDelimiters, &sub_save_ptr );
				val.iNormal = static_cast<uint32>( atoi( subtoken ) - 1 );

				save_ptr = sub_save_ptr + 1;
			};

			repeat( index.a );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			repeat( index.b );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			repeat( index.c );

			rawIndices.push_back( index );

			continue;
		}

		// move token to the next line
		token = save_ptr;
		while ( true )
		{
			if ( utl::IsNewline( *token ) || *token == '\0' )
			{
				// Skip forward
				++token;
				// Emulate strtok
				save_ptr = token;
				break;
			}
			++token;
		}
	}

	double end = Time_FloatMilliseconds();

	Com_DPrintf( "Obj positions: %zu\n", rawPositions.size() );
	Com_DPrintf( "Obj texcoords: %zu\n", rawCoords.size() );
	Com_DPrintf( "Obj normals  : %zu\n", rawNormals.size() );
	Com_DPrintf( "Obj indices  : %zu\n", rawIndices.size() );

	Com_Printf( "Parsed obj in %g milliseconds\n", end - start );

	//
	// generate raw tangents and map them to rawIndices
	//

	std::vector<vec3> rawTangents;		// raw face tangents

	rawTangents.reserve( rawIndices.size() );

	start = Time_FloatMilliseconds();

	uint degenerateDivisors = 0;

	for ( size_t iter1 = 0; iter1 < rawIndices.size(); ++iter1 )
	{
		vec3 &p1 = rawPositions[rawIndices[iter1].a.iPosition];
		vec3 &p2 = rawPositions[rawIndices[iter1].b.iPosition];
		vec3 &p3 = rawPositions[rawIndices[iter1].c.iPosition];

		vec2 &t1 = rawCoords[rawIndices[iter1].a.iTexcoord];
		vec2 &t2 = rawCoords[rawIndices[iter1].b.iTexcoord];
		vec2 &t3 = rawCoords[rawIndices[iter1].c.iTexcoord];

#if 1
		vec3 edge1, edge2;
		vec2 deltaUV1, deltaUV2;

		Vec3Subtract( p2, p1, edge1 );
		Vec3Subtract( p3, p1, edge2 );
		Vec2Subtract( t2, t1, deltaUV1 );
		Vec2Subtract( t3, t1, deltaUV2 );

		float divisor = ( deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y );
		if ( divisor == 0.0f )
		{
			++degenerateDivisors;
			divisor = FLT_EPSILON;
		}

		float f = 1.0f / divisor;
		assert( isfinite( f ) );

		// tangent for this set
		vec3 tangent;
		tangent.x = f * ( deltaUV2.y * edge1.x - deltaUV1.y * edge2.x );
		tangent.y = f * ( deltaUV2.y * edge1.y - deltaUV1.y * edge2.y );
		tangent.z = f * ( deltaUV2.y * edge1.z - deltaUV1.y * edge2.z );
		tangent.NormalizeFast();
#else
		vec3 tangent;

		CalcTriangleTangentSpace( p1, p2, p3, t1, t2, t3, tangent );
#endif

		rawTangents.push_back( tangent );

		// tangents map directly to rawIndices, don't need to do any indexing
	}

	end = Time_FloatMilliseconds();

	Com_Printf( "Generated face tangents in %g milliseconds\n", end - start );
	if ( degenerateDivisors != 0 )
	{
		Com_Printf( "WARNING: %u degenerate divisors\n", degenerateDivisors );
	}

	//
	// uncompress everything into a single array
	//

	m_indices.reserve( rawIndices.size() );

	start = Time_FloatMilliseconds();

	// for each index
	for ( size_t i = 0; i < rawIndices.size(); ++i )
	{
		auto repeat = [&]( const index_t &set ) -> uint32
		{
			uint32 index = 0;
			vertex_t newVertex;

			newVertex.position = rawPositions[set.iPosition];
			newVertex.texcoord = rawCoords[set.iTexcoord];
			newVertex.normal = rawNormals[set.iNormal];
			newVertex.tangent = rawTangents[i]; // note 'i' here

			// see if this combination already exists
			// TODO: slooooow
#if 1
			for ( uint32 k = 0; k < (uint32)m_vertices.size(); ++k )
			{
				//if ( memcmp( &newVertex, &m_vertices[k], sizeof( vertex_t ) ) == 0 )
				if ( Vec3Compare( newVertex.position, m_vertices[k].position ) &&
					 Vec2Compare( newVertex.texcoord, m_vertices[k].texcoord ) &&
					 Vec3Compare( newVertex.normal, m_vertices[k].normal ) )
				{
					// average the tangents

					Vec3Add( m_vertices[k].tangent, newVertex.tangent, m_vertices[k].tangent );

					index = k;
				}
			}
#endif

			if ( index == 0 )
			{
				// We're making a new combo
				m_vertices.push_back( newVertex );
				index = static_cast<uint32>( m_vertices.size() - 1 );
			}

			return index;
		};

		uint3 indices;

		indices.x = repeat( rawIndices[i].a );
		indices.y = repeat( rawIndices[i].b );
		indices.z = repeat( rawIndices[i].c );

		m_indices.push_back( indices );
	}

	end = Time_FloatMilliseconds();

	Com_Printf( "Generated optimised vertices and indices in %g milliseconds\n", end - start );
}
