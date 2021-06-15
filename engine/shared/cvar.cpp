/*
===================================================================================================

	Console commands

===================================================================================================
*/

#include "engine.h"

#include "cvar.h"

#define CVAR_USAGE_STR		1
#define CVAR_USAGE_DBL		2
#define CVAR_USAGE_I64		4

using cvarUsage_t = uint8;

cvar_t *cvar_vars;

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

cvar_t *Cvar_Find( const char *name )
{
	for ( cvar_t *var = cvar_vars; var; var = var->next )
	{
		if ( Q_strcmp( name, var->pName ) == 0 ) {
			return var;
		}
	}

	return nullptr;
}

float Cvar_VariableValue( const char *name )
{
	cvar_t *var = Cvar_Find( name );
	if ( !var ) {
		return 0.0f;
	}

	return var->GetFloat();
}

char *Cvar_VariableString( const char *name )
{
	cvar_t *var = Cvar_Find( name );
	if ( !var ) {
		return cvar_null_string;
	}

	return var->pString;
}

char *Cvar_CompleteVariable( const char *partial )
{
	strlen_t len = Q_strlen( partial );

	if ( len == 0 ) {
		return nullptr;
	}

	// check exact match
	for ( cvar_t *cvar = cvar_vars; cvar; cvar = cvar->next )
	{
		if ( Q_strcmp( partial, cvar->pName ) == 0 ) {
			return cvar->pName;
		}
	}

	// check partial match
	for ( cvar_t *cvar = cvar_vars; cvar; cvar = cvar->next )
	{
		if ( Q_strncmp( partial, cvar->pName, len ) == 0 ) {
			return cvar->pName;
		}
	}

	return nullptr;
}

// If the variable already exists, the value will not be set
// The flags will be or'ed in if the variable exists.
cvar_t *Cvar_Get( const char *name, const char *value, uint32 flags )
{
	if ( Q_strcmp( name, "s_volume" ) == 0  )
	{
		__debugbreak();
	}

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
	var->pName = Mem_CopyString( name );
	var->pString = Mem_CopyString( value );
	var->dblValue = atof( var->pString );
	var->i64Value = atoll( var->pString );

	// link the variable in
	var->next = cvar_vars;
	cvar_vars = var;

	var->flags = flags;

	var->SetModified();

	return var;
}

static cvar_t *Cvar_SetInternal( cvar_t *var, const char *value, bool force )
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
			Com_Printf( "%s is write protected.\n", var->pName );
			return var;
		}

		if ( var->flags & CVAR_LATCH )
		{
			if ( var->pLatchedString )
			{
				if ( Q_strcmp( value, var->pLatchedString ) == 0 ) {
					return var;
				}
				Mem_Free( var->pLatchedString );
			}
			else
			{
				if ( Q_strcmp( value, var->pString ) == 0 ) {
					return var;
				}
			}

			if ( Com_ServerState() )
			{
				Com_Printf( "%s will be changed for next game.\n", var->pName );
				var->pLatchedString = Mem_CopyString( value );
			}
			else
			{
				var->pString = Mem_CopyString( value );
				var->dblValue = atof( var->pString );
				var->i64Value = atoll( var->pString );

				// Hack?
				if ( Q_strcmp( var->pName, "game" ) == 0 )
				{
					FS_SetGamedir( var->pString, true );
					FS_ExecAutoexec();
				}
			}
			return var;
		}
	}
	else
	{
		if ( var->pLatchedString )
		{
			Mem_Free( var->pLatchedString );
			var->pLatchedString = NULL;
		}
	}

	if ( Q_strcmp( value, var->pString ) == 0 )
		return var;		// not changed

	if ( var->flags & CVAR_USERINFO )
		userinfo_modified = true;	// transmit at next oportunity

	Mem_Free( var->pString );	// free the old value string

	var->pString = Mem_CopyString( value );
	var->dblValue = atof( var->pString );
	var->i64Value = atoll( var->pString );

	var->SetModified();

	return var;
}

cvar_t *Cvar_ForceSet( const char *name, const char *value )
{
	cvar_t *var = Cvar_Find( name );
	if ( !var ) {
		// create it
		return Cvar_Get( name, value, 0 );
	}

	return Cvar_SetInternal( var, value, true );
}

cvar_t *Cvar_Set( const char *name, const char *value )
{
	cvar_t *var = Cvar_Find( name );
	if ( !var ) {
		// create it
		return Cvar_Get( name, value, 0 );
	}

	return Cvar_SetInternal( var, value, false );
}

cvar_t *Cvar_FullSet( const char *name, const char *value, uint32 flags )
{
	cvar_t *var = Cvar_Find( name );
	if ( !var ) {
		// create it
		return Cvar_Get( name, value, flags );
	}

	if ( var->flags & CVAR_USERINFO ) {
		// transmit at next opportunity
		userinfo_modified = true;
	}

	Mem_Free( var->pString );	// free the old value string

	var->pString = Mem_CopyString( value );
	var->dblValue = atof( var->pString );
	var->i64Value = atoll( var->pString );

	var->flags = flags;

	var->SetModified();

	return var;
}

void Cvar_SetValue( const char *var_name, float value )
{
	char val[128];

	if ( value == (int)value ) {
		Q_sprintf( val, "%d", (int)value );
	} else {
		Q_sprintf( val, "%f", value );
	}

	Cvar_Set( var_name, val );
}

void Cvar_SetInt64( cvar_t *var, int64 value )
{

}

void Cvar_SetInt32( cvar_t *var, int32 value )
{
	Cvar_SetInt64( var, value );
}

void Cvar_SetDouble( cvar_t *var, double value )
{

}

void Cvar_SetFloat( cvar_t *var, float value )
{
	Cvar_SetDouble( var, value );
}

// Any variables with latched values will now be updated
void Cvar_GetLatchedVars()
{
	for ( cvar_t *var = cvar_vars; var; var = var->next )
	{
		if ( !var->pLatchedString ) {
			continue;
		}

		Mem_Free( var->pString );
		var->pString = var->pLatchedString;
		var->pLatchedString = nullptr;
		var->dblValue = atof( var->pString );
		var->i64Value = atoll( var->pString );

		// Hack?
		if ( Q_strcmp( var->pName, "game" ) == 0 )
		{
			FS_SetGamedir( var->pString, true );
			FS_ExecAutoexec();
		}
	}
}

// Handles variable inspection and changing from the console
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
		Com_Printf( "\"%s\" is \"%s\"\n", var->pName, var->pString );
		return true;
	}

	Cvar_Set( var->pName, Cmd_Argv( 1 ) );

	return true;
}

// Appends lines containing "set variable value" for all variables
// with the archive flag set to true.
void Cvar_WriteVariables( FILE *f )
{
	char buffer[1024];

	for ( cvar_t *var = cvar_vars; var; var = var->next )
	{
		if ( var->flags & CVAR_ARCHIVE )
		{
			Q_sprintf_s( buffer, "set %s \"%s\"\n", var->pName, var->pString );
			fputs( buffer, f );
		}
	}
}

//=================================================================================================

bool userinfo_modified;

char *Cvar_BitInfo( uint32 bit )
{
	static char info[MAX_INFO_STRING];

	info[0] = 0;

	for ( cvar_t *var = cvar_vars; var; var = var->next )
	{
		if ( var->flags & bit ) {
			Info_SetValueForKey( info, var->pName, var->pString );
		}
	}

	return info;
}

// returns an info string containing all the CVAR_USERINFO cvars
char *Cvar_Userinfo()
{
	return Cvar_BitInfo( CVAR_USERINFO );
}

// returns an info string containing all the CVAR_SERVERINFO cvars
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
	int i = 0;

	for ( ; var; var = var->next, ++i )
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

		Com_Printf( " %s \"%s\"\n", var->pName, var->pString );
	}

	Com_Printf( "%d cvars\n", i );
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

	for ( ; var; var = var->next )
	{
		if ( lastVar ) {
			Mem_Free( lastVar );
		}

		lastVar = var;

		Mem_Free( var->pName );
		Mem_Free( var->pString );
		Mem_Free( var->pLatchedString );
	}
	Mem_Free( lastVar );
}
