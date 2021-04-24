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
	char *token, *save_ptr;

	float2 f2;
	float3 f3;
	index3_t index;

	std::vector<float3> vertices;	// Raw vertices
	std::vector<float2> tcoords;	// Raw texture coordinates
	std::vector<float3> vnormals;	// Raw vertex normals

	std::vector<index3_t> indices;

	// Delimiters: space, LF, CR, tab
	token = strtok_r( buffer, Delimiters, &save_ptr );
	for ( ; token && *token != '\0'; token = strtok_r( nullptr, Delimiters, &save_ptr ) )
	{
		// Vertex
		if ( strcmp( token, "v" ) == 0 )
		{
			// Skip past 'v'
			token = strtok_r( nullptr, Delimiters, &save_ptr );

			f3.x = (float)atof( token );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			f3.y = (float)atof( token );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			f3.z = (float)atof( token );

			vertices.push_back( f3 );
		}
		// Texture coordinates
		else if ( strcmp( token, "vt" ) == 0 )
		{
			// Vertex normals
			// Skip past 'vt'
			token = strtok_r( nullptr, Delimiters, &save_ptr );

			f2.x = (float)atof( token );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			f2.y = (float)atof( token );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
		//	f2.z = (float)atof( token );

			tcoords.push_back( f2 );
		}
		// Vertex normal
		else if ( strcmp( token, "vn" ) == 0 )
		{
			// Vertex normals
			// Skip past 'vn'
			token = strtok_r( nullptr, Delimiters, &save_ptr );

			f3.x = (float)atof( token );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			f3.y = (float)atof( token );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			f3.z = (float)atof( token );

			vnormals.push_back( f3 );
		}
		else if ( strcmp( token, "f" ) == 0 )
		{
			// Some indices
			// Skip past 'f'
			// Only support format: n/n/n
			token = strtok_r( nullptr, Delimiters, &save_ptr );

			auto repeat = [&]( index_t &val )
			{
				char *subtoken, *sub_save_ptr;

				subtoken = strtok_r( token, SubDelimiters, &sub_save_ptr );

				val.ivertex = (uint32)( atoll( subtoken ) - 1 );
				subtoken = strtok_r( nullptr, SubDelimiters, &sub_save_ptr );
				val.itexcoord = (uint32)( atoll( subtoken ) - 1 );
				subtoken = strtok_r( nullptr, SubDelimiters, &sub_save_ptr );
				val.inormal = (uint32)( atoll( subtoken ) - 1 );

				save_ptr = sub_save_ptr + 1;
			};

			repeat( index.a );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			repeat( index.b );
			token = strtok_r( nullptr, Delimiters, &save_ptr );
			repeat( index.c );

			indices.push_back( index );
		}
		else
		{
			// Move token to the next line
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
	}

	// Uncompress everything into a single array

#if 0

	// Create a combo for every index
	for ( size_t i = 0; i < indices.size(); ++i )
	{
		auto repeat = [&]( index_t &set )
		{
			vertex_t new_vertex;

			new_vertex.vertex = vertices[set.ivertex];
			new_vertex.texcoord = tcoords[set.itexcoord];
			new_vertex.normal = vnormals[set.inormal];

			m_vertices.push_back( new_vertex );
		};

		repeat( indices[i].a );
		repeat( indices[i].b );
		repeat( indices[i].c );
	}

#else

	for ( size_t i = 0; i < indices.size(); ++i )
	{
		auto repeat = [&]( const index_t &set ) -> uint32
		{
			uint32 index = 0;
			vertex_t new_vertex;

			new_vertex.vertex = vertices[set.ivertex];
			new_vertex.texcoord = tcoords[set.itexcoord];
			new_vertex.normal = vnormals[set.inormal];

			// See if this combo already exists
			for ( size_t k = 0; k < m_vertices.size(); ++k )
			{
				if ( memcmp( &new_vertex, &m_vertices[k], sizeof( vertex_t ) ) == 0 )
				{
					index = (uint32)k;
				}
			}

			if ( index == 0 )
			{
				// We're making a new combo
				m_vertices.push_back( new_vertex );
				index = (uint32)( m_vertices.size() - 1 );
			}

			return index;
		};

		uint3 indices2;

		indices2.x = repeat( indices[i].a );
		indices2.y = repeat( indices[i].b );
		indices2.z = repeat( indices[i].c );

		m_indices.push_back( indices2 );
	}

#endif
}
