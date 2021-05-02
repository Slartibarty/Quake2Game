//=================================================================================================
// Dynamic variable tracking
//=================================================================================================

#include "engine.h"

#include "cvar.h"

cvar_t *cvar_vars;

static char cvar_null_string[1]; // lame

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static bool Cvar_InfoValidate( const char *s )
{
	if ( strstr( s, "\\" ) )
		return false;
	if ( strstr( s, "\"" ) )
		return false;
	if ( strstr( s, ";" ) )
		return false;
	return true;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
cvar_t *Cvar_FindVar( const char *var_name )
{
	cvar_t *var;

	for ( var = cvar_vars; var; var = var->next )
	{
		if ( Q_strcmp( var_name, var->name ) == 0 )
			return var;
	}

	return nullptr;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
float Cvar_VariableValue( const char *var_name )
{
	cvar_t *var;

	var = Cvar_FindVar( var_name );
	if ( !var )
		return 0.0f;

	return var->value;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
char *Cvar_VariableString( const char *var_name )
{
	cvar_t *var;

	var = Cvar_FindVar( var_name );
	if ( !var )
		return cvar_null_string;

	return var->string;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
char *Cvar_CompleteVariable( const char *partial )
{
	cvar_t *cvar;
	strlen_t len;

	len = Q_strlen( partial );

	if ( len == 0 )
		return nullptr;

	// check exact match
	for ( cvar = cvar_vars; cvar; cvar = cvar->next )
		if ( Q_strcmp( partial, cvar->name ) == 0 )
			return cvar->name;

	// check partial match
	for ( cvar = cvar_vars; cvar; cvar = cvar->next )
		if ( Q_strncmp( partial, cvar->name, len ) == 0 )
			return cvar->name;

	return nullptr;
}

//-------------------------------------------------------------------------------------------------
// If the variable already exists, the value will not be set
// The flags will be or'ed in if the variable exists.
//-------------------------------------------------------------------------------------------------
cvar_t *Cvar_Get( const char *var_name, const char *var_value, uint32 flags )
{
	cvar_t *var;

	if ( flags & ( CVAR_USERINFO | CVAR_SERVERINFO ) )
	{
		if ( !Cvar_InfoValidate( var_name ) )
		{
			Com_Printf( "invalid info cvar name\n" );
			return nullptr;
		}
	}

	var = Cvar_FindVar( var_name );
	if ( var )
	{
		var->flags |= flags;
		return var;
	}

	if ( !var_value )
		return nullptr;

	if ( flags & ( CVAR_USERINFO | CVAR_SERVERINFO ) )
	{
		if ( !Cvar_InfoValidate( var_value ) )
		{
			Com_Printf( "invalid info cvar value\n" );
			return nullptr;
		}
	}

	var = (cvar_t *)Z_Calloc( sizeof( cvar_t ) );
	var->name = Z_CopyString( var_name );
	var->string = Z_CopyString( var_value );
	var->modified = true;
	var->value = (float)atof( var->string );

	// link the variable in
	var->next = cvar_vars;
	cvar_vars = var;

	var->flags = flags;

	return var;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
cvar_t *Cvar_Set2( const char *var_name, const char *value, bool force )
{
	cvar_t *var;

	var = Cvar_FindVar( var_name );
	if ( !var )
	{	// create it
		return Cvar_Get( var_name, value, 0 );
	}

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
			Com_Printf( "%s is write protected.\n", var_name );
			return var;
		}

		if ( var->flags & CVAR_LATCH )
		{
			if ( var->latched_string )
			{
				if ( Q_strcmp( value, var->latched_string ) == 0 )
					return var;
				Z_Free( var->latched_string );
			}
			else
			{
				if ( Q_strcmp( value, var->string ) == 0 )
					return var;
			}

			if ( Com_ServerState() )
			{
				Com_Printf( "%s will be changed for next game.\n", var_name );
				var->latched_string = Z_CopyString( value );
			}
			else
			{
				var->string = Z_CopyString( value );
				var->value = (float)atof( var->string );
				if ( Q_strcmp( var->name, "game" ) == 0 )
				{
					FS_SetGamedir( var->string, true );
					FS_ExecAutoexec();
				}
			}
			return var;
		}
	}
	else
	{
		if ( var->latched_string )
		{
			Z_Free( var->latched_string );
			var->latched_string = NULL;
		}
	}

	if ( Q_strcmp( value, var->string ) == 0 )
		return var;		// not changed

	var->modified = true;

	if ( var->flags & CVAR_USERINFO )
		userinfo_modified = true;	// transmit at next oportunity

	Z_Free( var->string );	// free the old value string

	var->string = Z_CopyString( value );
	var->value = (float)atof( var->string );

	return var;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
cvar_t *Cvar_ForceSet( const char *var_name, const char *value )
{
	return Cvar_Set2( var_name, value, true );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
cvar_t *Cvar_Set( const char *var_name, const char *value )
{
	return Cvar_Set2( var_name, value, false );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
cvar_t *Cvar_FullSet( const char *var_name, const char *value, uint32 flags )
{
	cvar_t *var;

	var = Cvar_FindVar( var_name );
	if ( !var )
	{	// create it
		return Cvar_Get( var_name, value, flags );
	}

	var->modified = true;

	if ( var->flags & CVAR_USERINFO )
		userinfo_modified = true;	// transmit at next opportunity

	Z_Free( var->string );	// free the old value string

	var->string = Z_CopyString( value );
	var->value = (float)atof( var->string );
	var->flags = flags;

	return var;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Cvar_SetValue( const char *var_name, float value )
{
	char val[64];

	if ( value == (int)value )
		Q_sprintf( val, "%i", (int)value );
	else
		Q_sprintf( val, "%f", value );

	Cvar_Set( var_name, val );
}

//-------------------------------------------------------------------------------------------------
// Any variables with latched values will now be updated
//-------------------------------------------------------------------------------------------------
void Cvar_GetLatchedVars()
{
	cvar_t *var;

	for ( var = cvar_vars; var; var = var->next )
	{
		if ( !var->latched_string )
			continue;

		Z_Free( var->string );
		var->string = var->latched_string;
		var->latched_string = nullptr;
		var->value = (float)atof( var->string );
		// Hack?
		if ( Q_strcmp( var->name, "game" ) == 0 )
		{
			FS_SetGamedir( var->string, true );
			FS_ExecAutoexec();
		}
	}
}

//-------------------------------------------------------------------------------------------------
// Handles variable inspection and changing from the console
//-------------------------------------------------------------------------------------------------
bool Cvar_Command()
{
	cvar_t *var;

	// check variables
	var = Cvar_FindVar( Cmd_Argv( 0 ) );
	if ( !var )
		return false;

	// perform a variable print or set
	if ( Cmd_Argc() == 1 )
	{
		Com_Printf( "\"%s\" is \"%s\"\n", var->name, var->string );
		return true;
	}

	Cvar_Set( var->name, Cmd_Argv( 1 ) );
	return true;
}

//-------------------------------------------------------------------------------------------------
// Allows setting and defining of arbitrary cvars from console
//-------------------------------------------------------------------------------------------------
void Cvar_Set_f()
{
	int c;
	int flags;

	c = Cmd_Argc();
	if ( c != 3 && c != 4 )
	{
		Com_Printf( "usage: set <variable> <value> [u / s]\n" );
		return;
	}

	if ( c == 4 )
	{
		if ( !Q_strcmp( Cmd_Argv( 3 ), "u" ) )
			flags = CVAR_USERINFO;
		else if ( !Q_strcmp( Cmd_Argv( 3 ), "s" ) )
			flags = CVAR_SERVERINFO;
		else
		{
			Com_Printf( "flags can only be 'u' or 's'\n" );
			return;
		}
		Cvar_FullSet( Cmd_Argv( 1 ), Cmd_Argv( 2 ), flags );
	}
	else
		Cvar_Set( Cmd_Argv( 1 ), Cmd_Argv( 2 ) );
}

//-------------------------------------------------------------------------------------------------
// Appends lines containing "set variable value" for all variables
// with the archive flag set to true.
//-------------------------------------------------------------------------------------------------
void Cvar_WriteVariables( const char *path )
{
	cvar_t *var;
	char buffer[1024];
	FILE *f;

	f = fopen( path, "a" );
	for ( var = cvar_vars; var; var = var->next )
	{
		if ( var->flags & CVAR_ARCHIVE )
		{
			Q_sprintf_s( buffer, "set %s \"%s\"\n", var->name, var->string );
			fputs( buffer, f );
		}
	}
	fclose( f );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Cvar_List_f()
{
	cvar_t *var;
	int i;

	i = 0;
	for ( var = cvar_vars; var; var = var->next, i++ )
	{
		if ( var->flags & CVAR_ARCHIVE )
			Com_Printf( "*" );
		else
			Com_Printf( " " );
		if ( var->flags & CVAR_USERINFO )
			Com_Printf( "U" );
		else
			Com_Printf( " " );
		if ( var->flags & CVAR_SERVERINFO )
			Com_Printf( "S" );
		else
			Com_Printf( " " );
		if ( var->flags & CVAR_NOSET )
			Com_Printf( "-" );
		else if ( var->flags & CVAR_LATCH )
			Com_Printf( "L" );
		else
			Com_Printf( " " );
		Com_Printf( " %s \"%s\"\n", var->name, var->string );
	}
	Com_Printf( "%i cvars\n", i );
}

//-------------------------------------------------------------------------------------------------

qboolean userinfo_modified;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
char *Cvar_BitInfo( uint32 bit )
{
	static char info[MAX_INFO_STRING];
	cvar_t *var;

	info[0] = 0;

	for ( var = cvar_vars; var; var = var->next )
	{
		if ( var->flags & bit )
			Info_SetValueForKey( info, var->name, var->string );
	}
	return info;
}

//-------------------------------------------------------------------------------------------------
// returns an info string containing all the CVAR_USERINFO cvars
//-------------------------------------------------------------------------------------------------
char *Cvar_Userinfo()
{
	return Cvar_BitInfo( CVAR_USERINFO );
}

//-------------------------------------------------------------------------------------------------
// returns an info string containing all the CVAR_SERVERINFO cvars
//-------------------------------------------------------------------------------------------------
char *Cvar_Serverinfo()
{
	return Cvar_BitInfo( CVAR_SERVERINFO );
}

//-------------------------------------------------------------------------------------------------
// Reads in all archived cvars
//-------------------------------------------------------------------------------------------------
void Cvar_Init()
{
	Cmd_AddCommand( "set", Cvar_Set_f );
	Cmd_AddCommand( "cvarlist", Cvar_List_f );
}
