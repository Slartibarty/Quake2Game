/*
===================================================================================================

	Console variables

	CVar names are case insensitive, values are case sensitive

	TODO: Move the misfit commands to cmdsystem?

===================================================================================================
*/

#include "framework_local.h"

// For the misfit commands
#include <vector>
#include <string>
#include <algorithm>

#include "cvarsystem.h"

cvar_t *cvar_vars;

bool userinfo_modified;

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

static void Cvar_SetValueWork( cvar_t *var, const char *newValue )
{
	const float newFltValue = Q_atof( newValue );
	const int newIntValue = Q_atoi( newValue ); // Just static cast newFltValue?

	const float min = var->minVal ? Q_atof( var->minVal ) : newFltValue;
	const float max = var->maxVal ? Q_atof( var->maxVal ) : newFltValue;

	if ( newFltValue < min ) {
		var->value.assign( var->minVal );
		var->fltValue = min;
		var->intValue = static_cast<int>( min );
		return;
	}
	else if ( newFltValue > max ) {
		var->value.assign( var->maxVal );
		var->fltValue = max;
		var->intValue = static_cast<int>( max );
		return;
	}

	// We didn't hit any limits, standard assign
	var->value.assign( newValue );
	var->fltValue = newFltValue;
	var->intValue = newIntValue;
}

static void Cvar_Add( cvar_t *var )
{
	// link the variable in
	var->pNext = cvar_vars;
	cvar_vars = var;
}

cvar_t *Cvar_Find( const char *name )
{
	// hashing doesn't help here, in fact it's somehow slower
	for ( cvar_t *var = cvar_vars; var; var = var->pNext )
	{
		if ( Q_stricmp( name, var->name.c_str() ) == 0 ) {
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
		if ( Q_stricmp( partial, cvar->name.c_str() ) == 0 ) {
			return cvar->name.data();
		}
	}

	// check partial match
	for ( cvar_t *cvar = cvar_vars; cvar; cvar = cvar->pNext )
	{
		if ( Q_strnicmp( partial, cvar->name.c_str(), len ) == 0 ) {
			return cvar->name.data();
		}
	}

	return nullptr;
}

//=================================================================================================

// If the variable already exists, the value will not be set
// The flags will be or'ed in if the variable exists.
cvar_t *Cvar_Get( const char *name, const char *value, uint32 flags, const char *help, changeCallback_t callback )
{
	Assert( name && name[0] && value);

	Assert( !( flags & CVAR_MODIFIED ) );

	if ( flags & ( CVAR_USERINFO | CVAR_SERVERINFO ) )
	{
		if ( !Cvar_InfoValidate( name ) )
		{
			Com_Print( S_COLOR_YELLOW "invalid info cvar name\n" );
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
		if ( callback )
		{
			// if no callback, make this the callback
			if ( !var->pCallback )
			{
				var->pCallback = callback;
			}
			else
			{
				Com_Printf( S_COLOR_YELLOW "Warning: cvar %s has callback defined in multiple locations\n", name );
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
			Com_Print( S_COLOR_YELLOW "invalid info cvar value\n" );
			return nullptr;
		}
	}

	var = (cvar_t *)Mem_ClearedAlloc( sizeof( cvar_t ) );

	var->name.assign( name );
	if ( help ) {
		var->help.assign( help );
	}
	if ( callback ) {
		var->pCallback = callback;
	}

	// Not supported for Cvar_Get vars yet
	var->minVal = nullptr; var->maxVal = nullptr;

	// Sets value, fltValue and intValue
	Cvar_SetValueWork( var, value );

	Cvar_Add( var );

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
			Com_Printf( S_COLOR_YELLOW "invalid info cvar value\n" );
			return var;
		}
	}

	char *oldString;
	float oldFloat;
	int oldInt;

	auto setOldValues = [&]()
	{
		if ( var->pCallback ) {
			// Save off the old values
			oldString = Mem_CopyString( var->value.c_str() );
			oldFloat = var->fltValue;
			oldInt = var->intValue;
		}
	};

	auto fireCallback = [&]()
	{
		if ( var->pCallback ) {
			var->pCallback( var, oldString, oldFloat, oldInt );
			Mem_Free( oldString );
		}
	};

	if ( !force )
	{
		if ( var->flags & CVAR_INIT )
		{
			Com_Printf( S_COLOR_YELLOW "%s is write protected.\n", var->name.c_str() );
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
				setOldValues();

				// Sets value, fltValue and intValue
				Cvar_SetValueWork( var, value );

				fireCallback();

				var->SetModified();
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

	setOldValues();

	// Sets value, fltValue and intValue
	Cvar_SetValueWork( var, value );

	fireCallback();

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

	var->flags = flags;

	Cvar_Set_Internal( var, value, true );
}

//=================================================================================================

void Cvar_SetString( cvar_t *var, const char *value )
{
	Cvar_Set_Internal( var, value, false );
}

void Cvar_SetFloat( cvar_t *var, float value )
{
	char str[32];
	Q_sprintf_s( str, "%f", value ); // %.6f
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
	Q_sprintf_s( str, "%f", value ); // %.6f

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
	char str[2]{ (char)value + '0', '\0' };

	cvar_t *var = Cvar_Find( name );
	if ( !var ) {
		// create it
		Cvar_Get( name, str, 0 );
		return;
	}

	Cvar_Set_Internal( var, str, false );
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

		Cvar_Set_Internal( var, var->latchedValue.c_str(), false );

		// FIXME: This doesn't call the callback... latch cvars suck

		// TODO: wtf?
		var->value = std::move( var->latchedValue );
		//Mem_Free( var->value );
		//var->value = var->latchedValue;
		//var->latchedValue = nullptr;

		// HACK HACK HACK: LATCHED VARS HAVE GOT TO GO!!!!
		var->fltValue = Q_atof( var->value.c_str() );
		var->intValue = Q_atoi( var->value.c_str() );
	}
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
	if ( flags & CVAR_INIT )
	{
		Com_Print( " init" );
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
		Com_Print( " modified" );
	}*/
	if ( flags & CVAR_STATIC )
	{
		Com_Print( " static" );
	}

	Com_Print( "\n" );
}

void Cvar_CallCallback( cvar_t *var )
{
	if ( var->pCallback )
	{
		var->pCallback( var, var->GetString(), var->GetFloat(), var->GetInt() );
	}
	else
	{
		Com_Printf( S_COLOR_YELLOW "[CvarSystem] Tried to call callback for \"%s\", but it doesn't have one!", var->GetName() );
	}
}

void Cvar_WriteVariables( fsHandle_t handle )
{
	char buffer[1024];

	for ( cvar_t *var = cvar_vars; var; var = var->pNext )
	{
		if ( var->flags & CVAR_ARCHIVE )
		{
			int bufferLen = Q_sprintf_s( buffer, "set %s \"%s\"\n", var->name.c_str(), var->value.c_str() );
			FileSystem::WriteFile( buffer, bufferLen, handle );
		}
	}
}

// This should probably be moved elsewhere

void Cvar_AddEarlyCommands( int argc, char **argv )
{
	for ( int i = 1; i < argc; ++i )
	{
		if ( Q_strcmp( argv[i], "+set" ) == 0 && ( i + 2 ) < argc )
		{
			Cvar_FindSetString( argv[i + 1], argv[i + 2] );
			i += 2;
		}
	}
}

//=================================================================================================

#ifdef Q_ENGINE

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

#endif

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

		if ( Q_strcmp( Cmd_Argv( 3 ), "u" ) == 0 )
		{
			flags = CVAR_USERINFO;
		}
		else if ( Q_strcmp( Cmd_Argv( 3 ), "s" ) == 0 )
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
		Cvar_FindSetString( Cmd_Argv( 1 ), Cmd_Argv( 2 ) );
	}
}

static void Cvar_Toggle_f()
{
	if ( Cmd_Argc() != 2 )
	{
		Com_Print( "Bad parameters\n" );
		return;
	}

	const char *cmdName = Cmd_Argv( 1 );
	cvar_t *var = Cvar_Find( cmdName );
	if ( !var )
	{
		Com_Printf( "Cvar \"%s\" does not exist\n", cmdName );
		return;
	}

	Cvar_SetBool( var, !var->GetBool() );

	Com_Printf( "Toggled %s to %s\n", var->GetName(), var->GetBool() ? "true" : "false" );
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

		if ( var->flags & CVAR_INIT ) {
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

// For lack of a better place, these are here

static void Find_f()
{
	if ( Cmd_Argc() != 2 )
	{
		Com_Print( "Usage: find <cmd/cvar/*>\n" );
		return;
	}

	const char *searchName = Cmd_Argv( 1 );
	if ( !searchName[0] )
	{
		return;
	}

	bool listAll = searchName[0] == '*' ? true : false;

	std::vector<cmdFunction_t *> cmdMatches;

	strlen_t longestName = 16;
	strlen_t longestValue = 8;
	strlen_t longestHelp = 8;

	// find command matches
	for ( cmdFunction_t *pCmd = cmd_functions; pCmd; pCmd = pCmd->pNext )
	{
		if ( listAll || Q_stristr( pCmd->pName, searchName ) )
		{
			strlen_t length = Q_strlen( pCmd->pName );
			if ( length > longestName )
			{
				longestName = length;
			}
			length = pCmd->pHelp ? Q_strlen( pCmd->pHelp ) : 0;
			if ( length > longestHelp )
			{
				longestHelp = length + 1;
			}
			cmdMatches.push_back( pCmd );
		}
	}

	//std::sort( cmdMatches.begin(), cmdMatches.end() );

	std::vector<cvar_t *> cvarMatches;

	// find cvar matches
	for ( cvar_t *pVar = cvar_vars; pVar; pVar = pVar->pNext )
	{
		if ( listAll || Q_stristr( pVar->GetName(), searchName ) )
		{
			if ( pVar->name.length() > longestName )
			{
				longestName = pVar->name.length() + 1;
			}
			if ( pVar->value.length() > longestValue )
			{
				longestValue = pVar->value.length() + 1;
			}
			if ( pVar->help.length() > longestHelp )
			{
				longestHelp = pVar->help.length() + 1;
			}
			cvarMatches.push_back( pVar );
		}
	}

	//std::sort( cvarMatches.begin(), cvarMatches.end() );

	// build the format string

	char formatString[128];
	char formatString2[128];	// To compensate for the green cmd names...
	Q_sprintf_s( formatString, "%%-%us" "%%-%us" "%%-%us\n", longestName, longestValue, longestHelp );
	Q_sprintf_s( formatString2, "%%-%us" "%%-%us" "%%-%us\n", longestName + 2, longestValue + 2, longestHelp );

	Com_Printf( formatString, "name", "value", "help text" );

	strlen_t maxLength = Min<strlen_t>( longestName + longestValue + longestHelp, 127 );

	char underscores[128]{};
	for ( strlen_t i = 0; ; ++i )
	{
		// HACK?
		if ( i == longestName - 1 || i == longestName + longestValue - 1 )
		{
			underscores[i] = ' ';
			continue;
		}
		underscores[i] = '_';
		if ( i == maxLength - 1 )
		{
			underscores[i] = '\n';
			break;
		}
	}

	Com_Print( underscores );

	std::string cmdNameGreen;

	for ( const cmdFunction_t *pCmd : cmdMatches )
	{
		cmdNameGreen.assign( S_COLOR_GREEN );
		cmdNameGreen.append( pCmd->pName );
		Com_Printf( formatString2, cmdNameGreen.c_str(), S_COLOR_WHITE "", pCmd->pHelp ? pCmd->pHelp : "" );
	}

	for ( const cvar_t *pVar : cvarMatches )
	{
		const char *string = pVar->GetString();
		if ( string[0] == '\0' )
		{
			string = "\"\"";
		}
		Com_Printf( formatString, pVar->GetName(), string, pVar->GetHelp() ? pVar->GetHelp() : "" );
	}

}

static void Help_f()
{
	if ( Cmd_Argc() != 2 )
	{
		Com_Print( "Usage: help <cmd/cvar>\n" );
		return;
	}

	const char *name = Cmd_Argv( 1 );
	if ( !name[0] )
	{
		return;
	}

	// search cmds
	for ( cmdFunction_t *pCmd = cmd_functions; pCmd; pCmd = pCmd->pNext )
	{
		if ( Q_stricmp( pCmd->pName, name ) == 0 )
		{
			Com_Printf( " - %s\n", pCmd->pHelp );
		}
	}

	// search cvars
	for ( cvar_t *pVar = cvar_vars; pVar; pVar = pVar->pNext )
	{
		if ( Q_stricmp( pVar->GetName(), name ) == 0 )
		{
			Cvar_PrintValue( pVar );
			Cvar_PrintFlags( pVar );
			Cvar_PrintHelp( pVar );
		}
	}
}

void Cvar_Init()
{
	Cmd_AddCommand( "set", Cvar_Set_f, "Interesting magical command." );
	Cmd_AddCommand( "toggle", Cvar_Toggle_f, "Toggles a convar as if it were a bool." );
	Cmd_AddCommand( "cvarList", Cvar_List_f, "Lists all cvars." );

	// Misfit commands
	Cmd_AddCommand( "find", Find_f, "Finds all cmds and cvars with <param> in the name, use * to list all." );
	Cmd_AddCommand( "help", Help_f, "Displays help for a given cmd or cvar." );
}

void Cvar_Shutdown()
{
	while ( cvar_vars )
	{
		cvar_t *pNext = cvar_vars->pNext;
		if ( !( cvar_vars->flags & CVAR_STATIC ) )
		{
			// FIXME: LMFAO :-)
			cvar_vars->name.~string();
			cvar_vars->value.~string();
			cvar_vars->help.~string();
			cvar_vars->latchedValue.~string();
			Mem_Free( cvar_vars );
		}
		cvar_vars = pNext;
	}
}

/*
===================================================================================================

	Statically defined cvars

===================================================================================================
*/

StaticCvar::StaticCvar( const char *Name, const char *Value, uint32 Flags, const char *Help,
	changeCallback_t Callback, const char *MinVal, const char *MaxVal )
{
	Assert( Name && Name[0] && Value );
	Assert( !( flags & CVAR_MODIFIED ) );

#ifdef Q_DEBUG
	if ( Cvar_Find( Name ) )
	{
		// Should be safe to call these before main?
		AssertMsg( false, "Multiply defined static cvar!" );
		exit( EXIT_FAILURE );
	}
#endif

	name.assign( Name );
	minVal = MinVal; maxVal = MaxVal;
	flags = Flags |= CVAR_STATIC;

	if ( Help ) help.assign( Help );
	if ( Callback ) pCallback = Callback;

	// Sets value, fltValue and intValue
	Cvar_SetValueWork( this, Value );

	// all newly created vars are "modified"
	SetModified();

	Cvar_Add( this );
}
