//=================================================================================================
// Think Tank 2
// Copyright (c) Slartibarty. All Rights Reserved.
//=================================================================================================

#pragma once

#ifdef THINKTANK

#include "Sys_Types.h"

#else // Quake 2

#include "q_types.h"

#endif

#include <cstring>		// memcpy

//-------------------------------------------------------------------------------------------------
// TANK STRING
//-------------------------------------------------------------------------------------------------

namespace tnk
{
	
	// 16 bytes
	class String
	{
	private:

		char	*m_Data;		// String
		uint32	m_Length;		// Size of the current string
		uint32	m_Size;			// Size of the internal array

	public:

		// Constructors

		// Default, zero out elements
		String()
			: m_Data(nullptr), m_Length(0), m_Size(0)
		{
		}

		// For runtime strings
		String( const char *string )
		{
			// Narrowing conversion
			uint32 size = static_cast<uint32>( strlen( string ) + 1 );

			m_Data = (char *)malloc( size );
			memcpy( m_Data, string, size );
			m_Length = size - 1;
			m_Size = size;
		}

		// For compile time strings
		template< uint32 size >
		String( char( &string )[size] )
		{
			m_Data = (char *)malloc( size );
			memcpy( m_Data, string, size );
			m_Length = size - 1;
			m_Size = size;
		}

		// Destructors

		~String()
		{
			// free on a nullptr does nothing, but I want the compiler to be able
			// to pick up and optimise cases where a String is never allocated in the first place
			if ( m_Data )
			{
				free( m_Data );
			}
		}

		// Assignment

		// Assign a c string to this
		void Assign( const char *string )
		{
			// Narrowing conversion
			uint32 size = static_cast<uint32>( strlen( string ) + 1 );

			// Only re-allocate data if we can't fit this new string into the existing array
			if ( size > m_Size )
			{
				m_Data = (char *)realloc( m_Data, size );
				m_Size = size;
			}

			memcpy( m_Data, string, size );
			m_Length = size - 1;
		}

		// Reset this String
		void Free()
		{
			// Free on a nullptr does nothing luckily
			assert( m_Data );

			free( m_Data );
			m_Data = nullptr;
			m_Length = 0;
			m_Size = 0;
		}

		// Convert a given value to a string
		template< typename T >
		void ToString( T value )
		{
			// HACK
			std::string fraud = std::to_string( value );

			if ( ( fraud.length() + 1 ) > m_Size )
			{
				m_Data = (char *)realloc( m_Data, fraud.length() + 1 );
				m_Size = static_cast<uint32>(fraud.length() + 1);
			}

			memcpy( m_Data, fraud.c_str(), fraud.length() + 1 );
			m_Length = static_cast<uint32>(fraud.length());
		}

		// Misc modifications

		// Shrink the internal array to fit the contained string
		void ShrinkToFit()
		{
			if ( ( m_Length + 1 ) < m_Size )
			{
				m_Data = (char *)realloc( m_Data, m_Length + 1 );
				m_Size = m_Length + 1;
			}
		}

		// Make the entire string lowercase
		void ToLower()
		{
			for ( char *iter = m_Data; *iter; ++iter )
			{
				*iter = tolower( *iter );
			}
		}

		// Make the entire string uppercase
		void ToUpper()
		{
			for ( char *iter = m_Data; *iter; ++iter )
			{
				*iter = toupper( *iter );
			}
		}

		// Substitute 'a' for 'b'
		void Substitute( char a, char b )
		{
			for ( char *iter = m_Data; *iter; ++iter )
			{
				if ( *iter == a )
				{
					*iter = b;
				}
			}
		}

		// Convert all '\' to '/'
		void WinPathToUnix()
		{
			Substitute( '\\', '/' );
		}

		// Convert all '/' to '\'
		void UnixPathToWin()
		{
			Substitute( '/', '\\' );
		}

		// Misc

		// Compare against another string
		int Compare( const char *string ) const
		{
			return strcmp( m_Data, string );
		}

		// Compare against another String

		// Returns true if the string is empty
		bool Empty() const
		{
			return (bool)m_Data;
		}

		// Getters

		// Return the raw string
		const char *c_str() const
		{
			return m_Data;
		}

		// Return length of string
		size_t Length() const
		{
			return m_Length;
		}

	};

	static_assert( sizeof( String ) == 16 );

}
