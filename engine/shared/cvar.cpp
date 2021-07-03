/*
===================================================================================================

	Console variables

===================================================================================================
*/

#include "engine.h"

#include "cvar.h"

cvar_t *cvar_vars;

bool userinfo_modified;

static char cvar_null_string[1]; // lame

static bool Cvar_InfoValidate( const char *s )
{
	if ( strstr( s, "\\" ) ) {
		return false;
	}
	if ( strstr( s, "\"" ) ) {
		return false;
	}
	if ( strstr( s, ";" ) ) {
		return false;
	}

	return true;
}

static void Cvar_SetDerivatives( cvar_t *var )
{
	var->fltValue = static_cast<float>( atof( var->value.c_str() ) );
	var->intValue = atoi( var->value.c_str() );
}

cvar_t *Cvar_Find( const char *name )
{
	// hashing doesn't help here, in fact it's somehow slower
	for ( cvar_t *var = cvar_vars; var; var = var->pNext )
	{
		if ( Q_strcmp( name, var->name.c_str() ) == 0 ) {
			return var;
		}
	}

	return nullptr;
}

char *Cvar_CompleteVariable( const char *partial )
{
	strlen_t len = Q_strlen( partial );

	if ( len == 0 ) {
		return nullptr;
	}

	// check exact match
	for ( cvar_t *cvar = cvar_vars; cvar; cvar = cvar->pNext )
	{
		if ( Q_strcmp( partial, cvar->name.c_str() ) == 0 ) {
			return cvar->name.data();
		}
	}

	// check partial match
	for ( cvar_t *cvar = cvar_vars; cvar; cvar = cvar->pNext )
	{
		if ( Q_strncmp( partial, cvar->name.c_str(), len ) == 0 ) {
			return cvar->name.data();
		}
	}

	return nullptr;
}

//=================================================================================================

// If the variable already exists, the value will not be set
// The flags will be or'ed in if the variable exists.
cvar_t *Cvar_Get( const char *name, const char *value, uint32 flags, const char *help )
{
	assert( name );
	assert( value );

	if ( flags & ( CVAR_USERINFO | CVAR_SERVERINFO ) )
	{
		if ( !Cvar_InfoValidate( name ) )
		{
			Com_Print( "invalid info cvar name\n" );
			return nullptr;
		}
	}

	cvar_t *var = Cvar_Find( name );
	if ( var )
	{
		if ( help )
		{
			// if no help, make this the help
			if ( var->help.empty() )
			{
				var->help.assign( help );
			}
			else
			{
				Com_Printf( S_COLOR_YELLOW "Warning: cvar %s has help defined in multiple locations\n", name );
			}
		}
		var->flags |= flags;
		return var;
	}

	if ( !value ) {
		return nullptr;
	}

	if ( flags & ( CVAR_USERINFO | CVAR_SERVERINFO ) )
	{
		if ( !Cvar_InfoValidate( value ) )
		{
			Com_Print( "invalid info cvar value\n" );
			return nullptr;
		}
	}

	var = (cvar_t *)Mem_ClearedAlloc( sizeof( cvar_t ) );

	var->name.assign( name );
	var->value.assign( value );
	if ( help ) {
		var->help.assign( help );
	}

	Cvar_SetDerivatives( var );

	// link the variable in
	var->pNext = cvar_vars;
	cvar_vars = var;

	var->flags = flags;

	// all newly created vars are "modified"
	var->SetModified();

	return var;
}

const char *Cvar_FindGetString( const char *name )
{
	cvar_t *var = Cvar_Find( name );
	if ( !var ) {
		return "";
	}

	return var->value.c_str();
}

float Cvar_FindGetFloat( const char *name )
{
	cvar_t *var = Cvar_Find( name );
	if ( !var ) {
		return 0.0f;
	}

	return var->GetFloat();
}

int Cvar_FindGetInt( const char *name )
{
	cvar_t *var = Cvar_Find( name );
	if ( !var ) {
		return 0;
	}

	return var->GetInt();
}

bool Cvar_FindGetBool( const char *name )
{
	return Cvar_FindGetInt( name ) != 0;
}

//=================================================================================================

static cvar_t *Cvar_Set_Internal( cvar_t *var, const char *value, bool force )
{
	if ( var->flags & ( CVAR_USERINFO | CVAR_SERVERINFO ) )
	{
		if ( !Cvar_InfoValidate( value ) )
		{
			Com_Printf( "invalid info cvar value\n" );
			return var;
		}
	}

	if ( !force )
	{
		if ( var->flags & CVAR_NOSET )
		{
			Com_Printf( "%s is write protected.\n", var->name.c_str() );
			return var;
		}

		if ( var->flags & CVAR_LATCH )
		{
			if ( !var->latchedValue.empty() )
			{
				if ( Q_strcmp( value, var->latchedValue.c_str() ) == 0 ) {
					return var;
				}
				var->latchedValue.clear();
			}
			else
			{
				if ( Q_strcmp( value, var->value.c_str() ) == 0 ) {
					return var;
				}
			}

			if ( Com_ServerState() )
			{
				Com_Printf( "%s will be changed for next game.\n", var->name.c_str() );

				var->latchedValue.assign( value );
			}
			else
			{
				var->value.assign( value );
				Cvar_SetDerivatives( var );

				// Hack?
				if ( Q_strcmp( var->name.c_str(), "fs_game" ) == 0 )
				{
					FS_SetGamedir( var->value.c_str(), true );
					FS_ExecAutoexec();
				}
			}

			return var;
		}
	}
	else
	{
		// forcing change... kill our latched data
		if ( !var->latchedValue.empty() )
		{
			var->latchedValue.clear();
		}
	}

	if ( Q_strcmp( value, var->value.c_str() ) == 0 ) {
		// not changed
		return var;
	}

	if ( var->flags & CVAR_USERINFO ) {
		// transmit at next opportunity
		userinfo_modified = true;
	}

	var->value.assign( value );
	Cvar_SetDerivatives( var );

	var->SetModified();

	return var;
}

void Cvar_ForceSet( cvar_t *var, const char *value )
{
	Cvar_Set_Internal( var, value, true );
}

void Cvar_FullSet( const char *name, const char *value, uint32 flags )
{
	cvar_t *var = Cvar_Find( name );
	if ( !var ) {
		// create it
		Cvar_Get( name, value, flags );
		return;
	}

	if ( var->flags & CVAR_USERINFO ) {
		// transmit at next opportunity
		userinfo_modified = true;
	}

	var->flags = flags;

	if ( Q_strcmp( value, var->value.c_str() ) != 0 ) {
		// value changed
		var->value.assign( value );
		Cvar_SetDerivatives( var );

		var->SetModified();
	}
}

//=================================================================================================

void Cvar_SetString( cvar_t *var, const char *value )
{
	Cvar_Set_Internal( var, value, false );
}

void Cvar_SetFloat( cvar_t *var, float value )
{
	char str[32];
	Q_sprintf_s( str, "%f", value); // %.6f
	Cvar_Set_Internal( var, str, false );
}

void Cvar_SetInt( cvar_t *var, int value )
{
	char str[16];
	Q_sprintf( str, "%d", value ); // safe
	Cvar_Set_Internal( var, str, false );
}

void Cvar_SetBool( cvar_t *var, bool value )
{
	char str[2]{ (char)value + '0', '\0' };
	Cvar_Set_Internal( var, str, false );
}

//=================================================================================================

void Cvar_FindSetString( const char *name, const char *value )
{
	cvar_t *var = Cvar_Find( name );
	if ( !var ) {
		// create it
		Cvar_Get( name, value, 0 );
		return;
	}

	Cvar_Set_Internal( var, value, false );
}

void Cvar_FindSetFloat( const char *name, float value )
{
	char str[128];
	Q_sprintf_s( str, "%f", value); // %.6f

	cvar_t *var = Cvar_Find( name );
	if ( !var ) {
		// create it
		Cvar_Get( name, str, 0 );
		return;
	}

	Cvar_Set_Internal( var, str, false );
}

void Cvar_FindSetInt( const char *name, int value )
{
	char str[16];
	Q_sprintf( str, "%d", value ); // safe

	cvar_t *var = Cvar_Find( name );
	if ( !var ) {
		// create it
		Cvar_Get( name, str, 0 );
		return;
	}

	Cvar_Set_Internal( var, str, false );
}

void Cvar_FindSetBool( const char *name, bool value )
{
	char str[2]{ (char)value, '\0' };

	cvar_t *var = Cvar_Find( name );
	if ( !var ) {
		// create it
		Cvar_Get( name, str, 0 );
		return;
	}

	Cvar_Set_Internal( var, str, false );
}

// LEGACY
void Cvar_Set( const char *name, const char *value )
{
	return Cvar_FindSetString( name, value );
}

//=================================================================================================

// Any variables with latched values will now be updated
void Cvar_GetLatchedVars()
{
	for ( cvar_t *var = cvar_vars; var; var = var->pNext )
	{
		if ( var->latchedValue.empty() ) {
			continue;
		}

		var->value = std::move( var->latchedValue );
		//Mem_Free( var->value );
		//var->value = var->latchedValue;
		//var->latchedValue = nullptr;

		Cvar_SetDerivatives( var );

		// Hack?
		if ( Q_strcmp( var->name.c_str(), "fs_game" ) == 0 )
		{
			FS_SetGamedir( var->value.c_str(), true );
			FS_ExecAutoexec();
		}
	}
}

bool Cvar_Command()
{
	// check variables
	cvar_t *var = Cvar_Find( Cmd_Argv( 0 ) );
	if ( !var ) {
		return false;
	}

	// perform a variable print or set
	if ( Cmd_Argc() == 1 )
	{
		Cvar_PrintValue( var );
		Cvar_PrintFlags( var );
		Cvar_PrintHelp( var );
		return true;
	}

	Cvar_Set( var->name.c_str(), Cmd_Argv( 1 ) );

	return true;
}

void Cvar_PrintValue( cvar_t *var )
{
	Com_Printf( "\"%s\" is \"%s\"\n", var->GetName(), var->GetString() );
}

void Cvar_PrintHelp( cvar_t *var )
{
	if ( !var->help.empty() ) {
		Com_Printf( " - %s\n", var->help.c_str() );
	}
}

void Cvar_PrintFlags( cvar_t *var )
{
	uint32 flags = var->flags & ~CVAR_MODIFIED;

	if ( flags == 0 )
	{
		return;
	}

	if ( flags & CVAR_ARCHIVE )
	{
		Com_Print( " archive" );
	}
	if ( flags & CVAR_USERINFO )
	{
		Com_Print( " userinfo" );
	}
	if ( flags & CVAR_SERVERINFO )
	{
		Com_Print( " serverinfo" );
	}
	if ( flags & CVAR_NOSET )
	{
		Com_Print( " noset" );
	}
	if ( flags & CVAR_LATCH )
	{
		Com_Print( " latch" );
	}
	if ( flags & CVAR_CHEAT )
	{
		Com_Print( " cheat" );
	}
	/*if ( flags & CVAR_MODIFIED )
	{
		Com_Print( "archive" );
	}*/

	Com_Print( "\n" );
}

void Cvar_WriteVariables( FILE *f )
{
	char buffer[1024];

	for ( cvar_t *var = cvar_vars; var; var = var->pNext )
	{
		if ( var->flags & CVAR_ARCHIVE )
		{
			Q_sprintf_s( buffer, "set %s \"%s\"\n", var->name.c_str(), var->value.c_str() );
			fputs( buffer, f );
		}
	}
}

//=================================================================================================

static char *Cvar_BitInfo( uint32 bit )
{
	static char info[MAX_INFO_STRING];

	info[0] = 0;

	for ( cvar_t *var = cvar_vars; var; var = var->pNext )
	{
		if ( var->flags & bit ) {
			Info_SetValueForKey( info, var->name.c_str(), var->value.c_str() );
		}
	}

	return info;
}

char *Cvar_Userinfo()
{
	return Cvar_BitInfo( CVAR_USERINFO );
}

char *Cvar_Serverinfo()
{
	return Cvar_BitInfo( CVAR_SERVERINFO );
}

// Allows setting and defining of arbitrary cvars from console
void Cvar_Set_f()
{
	int count = Cmd_Argc();
	if ( count != 3 && count != 4 )
	{
		Com_Print( "usage: set <variable> <value> [u / s]\n" );
		return;
	}

	if ( count == 4 )
	{
		int flags;

		if ( !Q_strcmp( Cmd_Argv( 3 ), "u" ) )
		{
			flags = CVAR_USERINFO;
		}
		else if ( !Q_strcmp( Cmd_Argv( 3 ), "s" ) )
		{
			flags = CVAR_SERVERINFO;
		}
		else
		{
			Com_Print( "flags can only be 'u' or 's'\n" );
			return;
		}

		Cvar_FullSet( Cmd_Argv( 1 ), Cmd_Argv( 2 ), flags );
	}
	else
	{
		Cvar_Set( Cmd_Argv( 1 ), Cmd_Argv( 2 ) );
	}
}

static void Cvar_List_f()
{
	cvar_t *var = cvar_vars;
	uint i = 0;

	for ( ; var; var = var->pNext, ++i )
	{
		if ( var->flags & CVAR_ARCHIVE ) {
			Com_Printf( "*" );
		} else {
			Com_Printf( " " );
		}

		if ( var->flags & CVAR_USERINFO ) {
			Com_Printf( "U" );
		} else {
			Com_Printf( " " );
		}

		if ( var->flags & CVAR_SERVERINFO ) {
			Com_Printf( "S" );
		} else {
			Com_Printf( " " );
		}

		if ( var->flags & CVAR_NOSET ) {
			Com_Printf( "-" );
		}
		else if ( var->flags & CVAR_LATCH ) {
			Com_Printf( "L" );
		} else {
			Com_Printf( " " );
		}

		Com_Printf( " %s \"%s\"\n", var->name.c_str(), var->value.c_str() );
	}

	Com_Printf( "%u cvars\n", i );
}

static void Cvar_Help_f()
{

}

void Cvar_Init()
{
	Cmd_AddCommand( "set", Cvar_Set_f );
	Cmd_AddCommand( "cvarlist", Cvar_List_f );
}

void Cvar_Shutdown()
{
	cvar_t *var = cvar_vars;
	cvar_t *lastVar = nullptr;

	for ( ; var; var = var->pNext )
	{
		if ( lastVar ) {
			Mem_Free( lastVar );
		}

		var->name.~string();
		var->value.~string();
		var->latchedValue.~string();
		lastVar = var;
	}
	Mem_Free( lastVar );
}
