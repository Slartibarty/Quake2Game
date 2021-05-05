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

			v2.x = static_cast<float>( atof( token ) );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			v2.y = static_cast<float>( 1.0 - atof( token ) ); // swap for opengl :)
			if ( !blenderMode ) {
				// 3ds max writes an extra 0.0 value
				token = strtok_r( nullptr, Delimiters, &save_ptr );
			}

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

	Com_Printf( "Parsed obj in %g milliseconds\n", end - start );

	//
	// generate raw tangents and map them to rawIndices
	//

	std::vector<vec3> rawTangents;		// raw face tangents

	rawTangents.reserve( rawIndices.size() );

	start = Time_FloatMilliseconds();

	for ( size_t iter1 = 0; iter1 < rawIndices.size(); ++iter1 )
	{
		vec3 &p1 = rawPositions[rawIndices[iter1].a.iPosition];
		vec3 &p2 = rawPositions[rawIndices[iter1].b.iPosition];
		vec3 &p3 = rawPositions[rawIndices[iter1].c.iPosition];

		vec2 &t1 = rawCoords[rawIndices[iter1].a.iTexcoord];
		vec2 &t2 = rawCoords[rawIndices[iter1].b.iTexcoord];
		vec2 &t3 = rawCoords[rawIndices[iter1].c.iTexcoord];

		vec3 edge1, edge2;
		vec2 deltaST1, deltaST2;

		Vec3Subtract( p2, p1, edge1 );
		Vec3Subtract( p3, p1, edge2 );
		Vec2Subtract( t2, t1, deltaST1 );
		Vec2Subtract( t3, t1, deltaST2 );

		float f = 1.0f / ( deltaST1.x * deltaST2.y - deltaST2.x * deltaST1.y );

		// tangent for this set
		vec3 tangent;

		tangent.x = f * ( deltaST2.y * edge1.x - deltaST1.y * edge2.x );
		tangent.y = f * ( deltaST2.y * edge1.y - deltaST1.y * edge2.y );
		tangent.z = f * ( deltaST2.y * edge1.z - deltaST1.y * edge2.z );

		rawTangents.push_back( tangent );

		// tangents map directly to rawIndices, don't need to do any indexing
	}

	end = Time_FloatMilliseconds();

	Com_Printf( "Generated face tangents in %g milliseconds\n", end - start );

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
			for ( size_t k = 0; k < m_vertices.size(); ++k )
			{
				//if ( memcmp( &newVertex, &m_vertices[k], sizeof( vertex_t ) ) == 0 )
				if ( Vec3Compare( newVertex.position, m_vertices[k].position ) &&
					 Vec2Compare( newVertex.texcoord, m_vertices[k].texcoord ) &&
					 Vec3Compare( newVertex.normal, m_vertices[k].normal ) )
				{
					// average the tangents

					Vec3Add( m_vertices[k].tangent, newVertex.tangent, m_vertices[k].tangent );

					index = static_cast<uint32>( k );
				}
			}

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
