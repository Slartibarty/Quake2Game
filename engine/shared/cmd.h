/*
===================================================================================================

	Quake script command processing module

	Provides command text buffering (Cbuf) and command execution (Cmd)

===================================================================================================
*/

#pragma once

/*
===============================================================================
	Command buffer

	Any number of commands can be added in a frame, from several different sources.
	Most commands come from either keybindings or console line input, but remote
	servers can also send across commands and entire text files can be execed.

	The + command line options are also added to the command buffer.

	The game starts with a CmdBuffer::AddText ("exec quake.rc\n"); CmdBuffer::Execute ();
===============================================================================
*/

namespace CmdBuffer
{

		// Allocates an initial text buffer that will grow as needed
void	Init();

		// As new commands are generated from the console or keybindings,
		// the text is added to the end of the command buffer.
void	AddText( const char *text );

		// When a command wants to issue other commands immediately, the text is
		// inserted at the beginning of the buffer, before any remaining unexecuted
		// commands.
void	InsertText( const char *text );

		// Adds all the +set commands from the command line.
void	AddEarlyCommands( int argc, char **argv );

		// Adds all the remaining + commands from the command line,
		// returns true if any late commands were added, which
		// will keep the demoloop from immediately starting.
bool	AddLateCommands( int argc, char **argv );

		// Pulls off \n terminated lines of text from the command buffer and sends
		// them through ExecuteString.  Stops when the buffer is empty.
		// Normally called once per frame, but may be explicitly invoked.
		// Do not call inside a command function!
void	Execute();

		// These two functions are used to defer any pending commands while a map
		// is being loaded.
void	CopyToDefer();
void	InsertFromDefer();

} // namespace CmdBuffer

/*
===============================================================================
	Command execution

	Command execution takes a null terminated string, breaks it into tokens,
	then searches for a command or variable that matches the first token.
===============================================================================
*/

namespace CmdSystem
{

typedef void ( *xCommand_t )( void );

// HACK: would be nice to find a way to iterate through commands without having this be visible
// this is only here to satisfy the console
struct cmdFunction_t
{
	const char *		pName;
	const char *		pHelp;
	xCommand_t			pFunction;
	cmdFunction_t *		pNext;
};

// possible commands to execute
extern cmdFunction_t *cmd_functions;

void			Init();
void			Shutdown();

				// called by the init functions of other parts of the program to
				// register commands and functions to call for them.
				// The cmd_name is referenced later, so it should not be in temp memory
				// if function is NULL, the command will be forwarded to the server
				// as a clc_stringcmd instead of executed locally
void			AddCommand( const char *cmd_name, xCommand_t function, const char *help = nullptr );
void			RemoveCommand( const char *cmd_name );

				// Used by the cvar code to check for cvar / command name overlap
bool			CommandExists( const char *cmd_name );

				// attempts to match a partial command for automatic command line completion
				// returns NULL if nothing fits
const char *	CompleteCommand( const char *partial );

				// The functions that execute commands get their parameters with these
				// functions. CmdSystem::GetArgv () will return an empty string, not a NULL
				// if arg > argc, so string operations are always safe.
int				GetArgc();
char *			GetArgv( int arg );
char *			GetArgs();

				// Takes a null terminated string. Does not need to be /n terminated.
				// Breaks the string up into arg tokens.
void			TokenizeString( char *text, bool macroExpand );

				// Parses a single line of text into arguments and tries to execute it
				// as if it was typed at the console.
void			ExecuteString( char *text );

} // namespace CmdSystem
