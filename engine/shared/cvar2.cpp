//=================================================================================================
// Convars 2
//
// This system should not care about the cvars themselves, it's up to the cvar objects to modify
// their own data, this just manages the array and provides basic functions to find convars.
// Convars are their own thing
//=================================================================================================

#if 0

// This has the Convar2 struct
#include "common.h"

Convar2System cvars;

// Private functions

bool Convar2System::InfoValidate( const char *s )
{
	/*if ( strstr( s, "\\" ) || strstr( s, "\"" ) || strstr( s, ";" ) )
	{
		return false;
	}*/
	for ( ; *s; ++s )
	{
		if ( *s == '\\' || *s == '\"' || *s == ';' )
		{
			return false;
		}
	}
	return true;
}

template< typename T >
Convar2 *Convar2System::InternalGet( const char *name, T value, uint32 flags )
{
	Convar2 *var;

	var = FindByName( name );
	if ( var )
	{
		var->nFlags |= flags;
		return var;
	}

	Convar2 newVar( name, value, flags );

	return &m_ConvarList.emplace_front( newVar );
}

template< typename T >
Convar2 *InternalSetByCvar( Convar2 *var, T value, bool force )
{
	var->bModified = true;
	
	if ( var->nFlags & ( CVAR_USERINFO | CVAR_SERVERINFO ) )
	{
		if ( !Cvar_InfoValidate( value ) )
		{
			Com_Printf( "invalid info cvar value\n" );
			return var;
		}
	}

}

template< typename T >
Convar2 *Convar2System::InternalSet( const char *name, T value, bool force )
{
	Convar2 *var;

	var = FindByName( name );
	if ( !var )
	{
		return nullptr;
	}

	return InternalSetByCvar( var, value, force );
}

// Public functions

const char *Convar2System::CompleteVariable( const char *partial )
{
	size_t length = strlen( partial );

	for ( auto &var : m_ConvarList )
	{
		if ( strncmp( var.GetName().c_str(), partial, length ) == 0 )
		{
			return var.GetName().c_str();
		}
	}

	return nullptr;
}

Convar2 *Convar2System::FindByName( const char *name )
{
	for ( auto &var : m_ConvarList )
	{
		if ( var.GetName().Compare( name ) == 0 )
		{
			return &var;
		}
	}

	return nullptr;
}

// Get by name

Convar2 *Convar2System::Get( const char *name, const char *value, uint32 flags )
{
	// Validate stuff for userinfo cvars
	if ( flags & ( CVAR2_USERINFO | CVAR2_SERVERINFO ) )
	{
		if ( !InfoValidate( name ) )
		{
			Com_Printf( "invalid info cvar name\n" );
			return nullptr;
		}
		if ( !InfoValidate( value ) )
		{
			Com_Printf( "invalid info cvar value\n" );
			return nullptr;
		}
	}

	return InternalGet( name, value, flags );
}

Convar2 *Convar2System::Get( const char *name, int64 value, uint32 flags )
{
	// Validate stuff for userinfo cvars
	if ( flags & ( CVAR2_USERINFO | CVAR2_SERVERINFO ) )
	{
		if ( !InfoValidate( name ) )
		{
			Com_Printf( "invalid info cvar name\n" );
			return nullptr;
		}
	}

	return InternalGet( name, value, flags );
}

Convar2 *Convar2System::Get( const char *name, double value, uint32 flags )
{
	// Validate stuff for userinfo cvars
	if ( flags & ( CVAR2_USERINFO | CVAR2_SERVERINFO ) )
	{
		if ( !InfoValidate( name ) )
		{
			Com_Printf( "invalid info cvar name\n" );
			return nullptr;
		}
	}

	return InternalGet( name, value, flags );
}

// Set by name

Convar2 *Convar2System::Set( const char *name, const char *value, bool force /* = false */ )
{
	Convar2 *var;

	var = FindByName( name );
	if ( !var )
	{
		// Create it
		Convar2 newVar( name, value, CVAR2_NONE );

		return &m_ConvarList.emplace_front( newVar );
	}

	if ( var->nFlags & ( CVAR2_USERINFO | CVAR2_SERVERINFO ) )
	{
		if ( !InfoValidate( value ) )
		{
			Com_Printf( "invalid info cvar value\n" );
			return var;
		}
	}

	return nullptr;
}

Convar2 *Convar2System::Set( const char *name, int64 value, bool force /* = false */ )
{
	Convar2 *var;

	var = FindByName( name );
	if ( !var )
	{
		// Create it
		Convar2 newVar( name, value, CVAR2_NONE );

		return &m_ConvarList.emplace_front( newVar );
	}

	return nullptr;
}

Convar2 *Convar2System::Set( const char *name, double value, bool force /* = false */ )
{
	Convar2 *var;

	var = FindByName( name );
	if ( !var )
	{
		// Create it
		Convar2 newVar( name, value, CVAR2_NONE );

		return &m_ConvarList.emplace_front( newVar );
	}

	return nullptr;
}

// Set by convar

Convar2 *Set( Convar2 *var, const char *value, bool force /* = false */ )
{
	return nullptr;
}

Convar2 *Set( Convar2 *var, int64 value, bool force /* = false */ )
{
	return nullptr;
}

Convar2 *Set( Convar2 *var, double value, bool force /* = false */ )
{
	return nullptr;
}

#endif
