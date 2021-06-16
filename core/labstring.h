/*
===================================================================================================

	Lab string

	Weakly based on std::string

===================================================================================================
*/

#pragma once

#include "sys_types.h"

#ifdef Q_DEBUG
//#define LABSTR_PARANOID
#endif

namespace lab
{
	class string
	{
	private:
		char *		m_pData;		// the string
		uint32		m_length;		// length of the current string
		uint32		m_capacity;		// allocated size

	public:
		string( string && ) = default;
		string &operator=( string && ) = default;

		string( const char *pString ) {
			m_length = (uint32)strlen( pString );

			m_capacity = m_length + 1;
			m_pData = (char *)malloc( m_capacity );

			strcpy( m_pData, pString );
		}

		~string() {
			if ( m_pData ) {
				free( m_pData );
			}
		}

		// assignment

		void assign( const char *pString ) {
			m_length = (uint32)strlen( pString );

			if ( m_capacity <= m_length ) {
				if ( m_pData ) {
					free( m_pData );
				}
				m_capacity = m_length + 1;
				m_pData = (char *)malloc( m_capacity );
			}

			strcpy( m_pData, pString );
		}

		// element access

		[[nodiscard]] const char *c_str() const {
			return m_pData;
		}

		[[nodiscard]] const char *data() const {
			return m_pData;
		}

		[[nodiscard]] char *data() {
			return m_pData;
		}

		// capacity

		[[nodiscard]] bool empty() const {
			return m_length == 0;
		}

		// NOT implementing size()

		[[nodiscard]] uint32 length() const {
			return m_length;
		}

		// TODO: UNFINISHED!!!!!
		void reserve( uint32 newCapacity ) {
			if ( m_capacity >= newCapacity ) {
				// ignore if we're trying to allocate smaller than we currently are
				return;
			}

			if ( m_pData ) {
				free( m_pData );
			}

			m_capacity = newCapacity;
			m_pData = (char *)malloc( newCapacity );
		}

		[[nodiscard]] uint32 capacity() const {
			return m_capacity;
		}

		void shrink_to_fit() {
			// TODO: only handles deletions
			if ( m_length == 0 ) {
				if ( m_pData ) {
					free( m_pData );
				}
			}
		}

		// operations

		void clear() {
#ifdef LABSTR_PARANOID
			memset( m_pData, 0, m_length );
#else
			m_pData[0] = '\0';
#endif
			m_length = 0;
		}

	};

	static_assert( sizeof( string ) == 16 );

}
