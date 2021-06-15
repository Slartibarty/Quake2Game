/*
===================================================================================================

	Console variables

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

			// attempts to match a partial variable name for command line completion
			// returns NULL if nothing fits
char *		Cvar_CompleteVariable( const char *partial );

//=================================================================================================

			// creates the variable if it doesn't exist, or returns the existing one
			// if it exists, the value will not be changed, but flags will be ORed in
			// that allows variables to be unarchived without needing bitflags
cvar_t *	Cvar_Get( const char *name, const char *value, uint32 flags );

			// returns 0 if not defined or non numeric
float		Cvar_FindGetFloat( const char *name );

			// returns an empty string if not defined
char *		Cvar_FindGetString( const char *name );

//=================================================================================================

			// will set the variable even if NOSET or LATCH
void		Cvar_ForceSet( cvar_t *var, const char *value );

void		Cvar_FullSet( const char *name, const char *value, uint32 flags );

			// expands value to a string and calls Cvar_Set
void		Cvar_SetString( cvar_t *var, const char *string );
void		Cvar_SetFloat( cvar_t *var, float value );
void		Cvar_SetInt( cvar_t *var, int value );
void		Cvar_SetBool( cvar_t *var, bool value );

			// finds a variable, creates it if it doesn't exist, and sets the value
void		Cvar_FindSetString( const char *name, const char *value );
void		Cvar_FindSetFloat( const char *name, float value );
void		Cvar_FindSetInt( const char *name, int value );
void		Cvar_FindSetBool( const char *name, bool value );

//=================================================================================================

			// any CVAR_LATCHED variables that have been set will now take effect
void		Cvar_GetLatchedVars();

			// Handles variable inspection and changing from the console
			//
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
