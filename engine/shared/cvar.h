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

// singly-linked list of all cvars
extern cvar_t *cvar_vars;

// this is set each time a CVAR_USERINFO variable is changed
// so that the client knows to send it to the server
extern bool userinfo_modified;

			// finds and returns a variable by name, returns null if it doesn't exist
cvar_t *	Cvar_Find( const char *name );

			// attempts to match a partially complete variable name to an existing
			// variable, returns null when no matches were found
char *		Cvar_CompleteVariable( const char *partial );

//=================================================================================================

			// creates the variable if it doesn't exist, or returns the existing one
			// if it exists, the value will not be changed, but flags will be ORed in
			// that allows variables to be unarchived without needing bitflags
cvar_t *	Cvar_Get( const char *name, const char *value, uint32 flags, const char *help = nullptr );

			// finds and returns the value of a variable
const char *Cvar_FindGetString( const char *name );			// returns an empty string if not found
float		Cvar_FindGetFloat( const char *name );
int			Cvar_FindGetInt( const char *name );
bool		Cvar_FindGetBool( const char *name );

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

			// for consistency
void		Cvar_PrintValue( cvar_t *var );
void		Cvar_PrintHelp( cvar_t *var );
void		Cvar_PrintFlags( cvar_t *var );

			// appends lines containing "set variable value" for all variables
			// with the archive flag set to true.
void 		Cvar_WriteVariables( fsHandle_t handle );

			// returns an info string containing all the CVAR_USERINFO cvars
char *		Cvar_Userinfo();
			// returns an info string containing all the CVAR_SERVERINFO cvars
char *		Cvar_Serverinfo();

void		Cvar_Init();
void		Cvar_Shutdown();
