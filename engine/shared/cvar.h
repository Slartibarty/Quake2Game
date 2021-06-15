/*
===================================================================================================

	Console commands

	Id developer notes:

	cvar_t variables are used to hold scalar or string variables that can be changed or displayed
	at the console or prog code as well as accessed directly in C code.

	The user can access cvars from the console in three ways:
	r_draworder			prints the current value
	r_draworder 0		sets the current value to 0
	set r_draworder 0	as above, but creates the cvar if not present
	Cvars are restricted from having the same names as commands to keep this
	interface from being ambiguous.

===================================================================================================
*/

#pragma once

extern cvar_t *cvar_vars;

cvar_t *	Cvar_Find( const char *name );

			// returns 0 if not defined or non numeric
float		Cvar_VariableValue( const char *name );

			// returns an empty string if not defined
char *		Cvar_VariableString( const char *name );

			// attempts to match a partial variable name for command line completion
			// returns NULL if nothing fits
char *		Cvar_CompleteVariable( const char *partial );

			// creates the variable if it doesn't exist, or returns the existing one
			// if it exists, the value will not be changed, but flags will be ORed in
			// that allows variables to be unarchived without needing bitflags
cvar_t *	Cvar_Get( const char *name, const char *value, uint32 flags );

			// will create the variable if it doesn't exist
cvar_t *	Cvar_Set( const char *name, const char *value );

			// will set the variable even if NOSET or LATCH
cvar_t *	Cvar_ForceSet( const char *name, const char *value );

cvar_t *	Cvar_FullSet( const char *name, const char *value, uint32 flags );

			// expands value to a string and calls Cvar_Set
void		Cvar_SetValue( const char *name, float value );
void		Cvar_SetInt64( cvar_t *name, int64 value );
void		Cvar_SetInt32( cvar_t *name, int32 value );
void		Cvar_SetDouble( cvar_t *name, double value );
void		Cvar_SetFloat( cvar_t *name, float value );
void		Cvar_SetBool( cvar_t *name, bool value );

			// any CVAR_LATCHED variables that have been set will now take effect
void		Cvar_GetLatchedVars();

			// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
			// command.  Returns true if the command was a variable reference that
			// was handled. (print or change)
bool		Cvar_Command();

			// appends lines containing "set variable value" for all variables
			// with the archive flag set to true.
void 		Cvar_WriteVariables( FILE *f );

			// returns an info string containing all the CVAR_USERINFO cvars
char *		Cvar_Userinfo();
			// returns an info string containing all the CVAR_SERVERINFO cvars
char *		Cvar_Serverinfo();

void		Cvar_Init();
void		Cvar_Shutdown();

// this is set each time a CVAR_USERINFO variable is changed
// so that the client knows to send it to the server
extern bool userinfo_modified;
