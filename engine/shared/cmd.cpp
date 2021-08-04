/*
===================================================================================================

	Quake script command processing module

	Provides command text buffering (Cbuf) and command execution (Cmd)
	Cmd names are case insensitive

===================================================================================================
*/

#include "engine.h"

#include "cmd.h"

#define	MAX_ALIAS_NAME		32
#define	ALIAS_LOOP_COUNT	16

// Adds the current command line as a clc_stringcmd to the client message.
// Things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.
// Slart: This is in the client...
void Cmd_ForwardToServer();

namespace CmdSystem
{
static int alias_count;		// for detecting runaway loops
}

namespace CmdBuffer
{

/*
===============================================================================
	Command buffer
===============================================================================
*/

static bool			cmd_wait;

static sizebuf_t	cmd_text;
static byte			cmd_text_buf[8192];

static byte			defer_text_buf[8192];

/*
========================
Init
========================
*/
void Init()
{
	SZ_Init( &cmd_text, cmd_text_buf, sizeof( cmd_text_buf ) );
}

/*
========================
AddText

Adds command text at the end of the buffer
========================
*/
void AddText( const char *text )
{
	int l = (int)strlen( text );

	if ( cmd_text.cursize + l >= cmd_text.maxsize )
	{
		Com_Print( "CmdBuffer::AddText: overflow\n" );
		return;
	}

	SZ_Write( &cmd_text, text, l );
}

/*
========================
InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
========================
*/
void InsertText( const char *text )
{
	char *temp;
	int templen;

	// copy off any commands still remaining in the exec buffer
	templen = cmd_text.cursize;
	if ( templen )
	{
		temp = (char*)Mem_StackAlloc( templen );
		memcpy( temp, cmd_text.data, templen );
		SZ_Clear( &cmd_text );
	}

	// add the entire text of the file
	AddText( text );

	// add the copied off data
	if ( templen )
	{
		SZ_Write( &cmd_text, temp, templen );
	}
}

/*
========================
CopyToDefer
========================
*/
void CopyToDefer()
{
	memcpy( defer_text_buf, cmd_text_buf, cmd_text.cursize );
	defer_text_buf[cmd_text.cursize] = '\0';
	cmd_text.cursize = '\0';
}

/*
========================
InsertFromDefer
========================
*/
void InsertFromDefer()
{
	InsertText( (char*)defer_text_buf );
	defer_text_buf[0] = '\0';
}

/*
========================
Execute
========================
*/
void Execute()
{
	int		i;
	char	*text;
	char	line[1024];
	int		quotes;

	CmdSystem::alias_count = 0;		// don't allow infinite alias loops

	while ( cmd_text.cursize )
	{
		// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
		for ( i = 0; i < cmd_text.cursize; i++ )
		{
			if ( text[i] == '"' )
				quotes++;
			if ( !( quotes & 1 ) && text[i] == ';' )
				break;	// don't break if inside a quoted string
			if ( text[i] == '\n' )
				break;
		}


		memcpy( line, text, i );
		line[i] = '\0';

		// delete the text from the command buffer and move remaining commands down
		// this is necessary because commands (exec, alias) can insert data at the
		// beginning of the text buffer

		if ( i == cmd_text.cursize )
		{
			cmd_text.cursize = 0;
		}
		else
		{
			i++;
			cmd_text.cursize -= i;
			memmove( text, text + i, cmd_text.cursize );
		}

		// execute the command line
		CmdSystem::ExecuteString( line );

		if ( cmd_wait )
		{
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait = false;
			break;
		}
	}
}

/*
========================
AddEarlyCommands

Adds command line parameters as script statements
Commands lead with a +, and continue until another +

Set commands are added early, so they are guaranteed to be set before
the client and server initialize for the first time.

Other commands are added late, after all initialization is complete.
========================
*/
void AddEarlyCommands( int argc, char **argv )
{
	for ( int i = 0; i < argc; ++i )
	{
		if ( Q_strcmp( argv[i], "+set" ) != 0 ) {
			continue;
		}
		AddText( va( "set %s %s\n", argv[i + 1], argv[i + 2] ) );
		i += 2;
	}
}

/*
========================
AddLateCommands

Adds command line parameters as script statements
Commands lead with a + and continue until another + or -
quake +vid_ref gl +map amlev1

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
========================
*/
bool AddLateCommands( int argc, char **argv )
{
	int i, j;

	// build the combined string to parse from
	int s = 0;
	for ( i = 1; i < argc; ++i )
	{
		if ( Q_strcmp( argv[i], "+set" ) == 0 ) {
			i += 2;
			continue;
		}
		s += static_cast<int>( strlen( argv[i] ) + 1 );
	}
	if ( s == 0 ) {
		return false;
	}

	char *text = (char *)Mem_StackAlloc( s + 1 );
	text[0] = '\0';
	for ( i = 1; i < argc; ++i )
	{
		if ( Q_strcmp( argv[i], "+set" ) == 0 ) {
			i += 2;
			continue;
		}
		strcat( text, argv[i] );
		if ( i != argc - 1 ) {
			strcat( text, " " );
		}
	}

	// pull out the commands
	char *build = (char *)Mem_StackAlloc( s + 1 );
	build[0] = '\0';
	for ( i = 0; i < s - 1; ++i )
	{
		if ( text[i] == '+' )
		{
			++i;

			for ( j = i; ( text[j] != '+' ) && ( text[j] != '-' ) && ( text[j] != '\0' ); j++ )
				;

			char c = text[j];
			text[j] = '\0';

			strcat( build, text + i );
			strcat( build, "\n" );
			text[j] = c;
			i = j - 1;
		}
	}

	bool ret = build[0] != 0;
	if ( ret ) {
		AddText( build );
	}

	return ret;
}

} // namespace CommandBuffer

/*
===================================================================================================

	Command Execution

===================================================================================================
*/

namespace CmdSystem
{

struct cmdAlias_t
{
	char			name[MAX_ALIAS_NAME];
	char *			pValue;
	cmdAlias_t *	pNext;
};

// singly linked list of command aliases
static cmdAlias_t *cmd_alias;

static int		cmd_argc;
static char *	cmd_argv[MAX_STRING_TOKENS];
static char		cmd_args[MAX_STRING_CHARS];

// possible commands to execute
cmdFunction_t *cmd_functions;

/*
========================
GetArgc
========================
*/
int GetArgc()
{
	return cmd_argc;
}

/*
========================
GetArgv
========================
*/
char *GetArgv( int arg )
{
	static char null_char;

	assert( arg >= 0 && arg < cmd_argc );
	if ( arg >= cmd_argc ) {
		return &null_char;
	}
	return cmd_argv[arg];
}

/*
========================
GetArgs

Returns a single string containing argv(1) to argv(argc()-1)
========================
*/
char *GetArgs()
{
	return cmd_args;
}

/*
========================
MacroExpandString
========================
*/
static char *MacroExpandString( char *text )
{
	static char expanded[MAX_STRING_CHARS];
	char temporary[MAX_STRING_CHARS];

	char *scan = text;

	int len = static_cast<int>( strlen( scan ) );
	if ( len >= MAX_STRING_CHARS )
	{
		Com_Printf( "Line exceeded %i chars, discarded.\n", MAX_STRING_CHARS );
		return nullptr;
	}

	bool inquote = false;
	int count = 0;

	for ( int i = 0; i < len; i++ )
	{
		if ( scan[i] == '"' ) {
			inquote ^= 1;
		}
		if ( inquote ) {
			// don't expand inside quotes
			continue;
		}
		if ( scan[i] != '$' ) {
			continue;
		}

		// scan out the complete macro
		char *start = scan + i + 1;
		const char *token = COM_Parse( &start );
		if ( !start ) {
			continue;
		}

		token = Cvar_FindGetString( token );

		int j = static_cast<int>( strlen( token ) );
		len += j;
		if ( len >= MAX_STRING_CHARS )
		{
			Com_Printf( "Expanded line exceeded %i chars, discarded.\n", MAX_STRING_CHARS );
			return nullptr;
		}

		strncpy( temporary, scan, i );
		strcpy( temporary + i, token );
		strcpy( temporary + i + j, start );

		strcpy( expanded, temporary );
		scan = expanded;
		i--;

		if ( ++count == 100 )
		{
			Com_Print( "Macro expansion loop, discarded.\n" );
			return nullptr;
		}
	}

	if ( inquote )
	{
		Com_Print( "Line has unmatched quote, discarded.\n" );
		return nullptr;
	}

	return scan;
}

/*
========================
TokenizeString

Parses the given string into command line tokens.
$Cvars will be expanded unless they are in a quoted token
========================
*/
void TokenizeString( char *text, bool macroExpand )
{
	// clear the args from the last string
	for ( int i = 0; i < cmd_argc; i++ ) {
		Mem_Free( cmd_argv[i] );
	}

	cmd_argc = 0;
	cmd_args[0] = 0;

	// macro expand the text
	if ( macroExpand ) {
		text = MacroExpandString( text );
	}
	if ( !text ) {
		return;
	}

	while ( 1 )
	{
		// skip whitespace up to a /n
		while ( *text && *text <= ' ' && *text != '\n' )
		{
			text++;
		}

		if ( *text == '\n' )
		{
			// a newline seperates commands in the buffer
			text++;
			break;
		}

		if ( !*text ) {
			return;
		}

		// set cmd_args to everything after the first arg
		if ( cmd_argc == 1 )
		{
			strcpy( cmd_args, text );

			// strip off any trailing whitespace
			int l = static_cast<int>( strlen( cmd_args ) - 1 );
			for ( ; l >= 0; l-- )
			{
				if ( cmd_args[l] <= ' ' ) {
					cmd_args[l] = '\0';
				}
				else {
					break;
				}
			}
		}

		const char *com_token = COM_Parse( &text );
		if ( !text ) {
			return;
		}

		if ( cmd_argc < MAX_STRING_TOKENS )
		{
			cmd_argv[cmd_argc] = Mem_CopyString( com_token );
			cmd_argc++;
		}
	}
}

/*
========================
AddCommand
========================
*/
void AddCommand( const char *cmd_name, xCommand_t function, const char *help /*= nullptr*/ )
{
	// fail if the command is a variable name
	if ( Cvar_Find( cmd_name ) )
	{
		Com_Printf( "CmdSystem::AddCommand: %s already defined as a var\n", cmd_name );
		return;
	}

	cmdFunction_t *pCmd;

	// fail if the command already exists
	for ( pCmd = cmd_functions; pCmd; pCmd = pCmd->pNext )
	{
		if ( Q_stricmp( cmd_name, pCmd->pName ) == 0 )
		{
			Com_Printf( "CmdSystem::AddCommand: %s already defined as %s\n", cmd_name, pCmd->pName );
			return;
		}
	}

	pCmd = (cmdFunction_t *)Mem_Alloc( sizeof( cmdFunction_t ) );
	pCmd->pName = cmd_name;
	pCmd->pHelp = help;
	pCmd->pFunction = function;
	pCmd->pNext = cmd_functions;
	cmd_functions = pCmd;
}

/*
========================
RemoveCommand
========================
*/
void RemoveCommand( const char *cmd_name )
{
	cmdFunction_t *pCmd, **ppBack;

	ppBack = &cmd_functions;
	while ( 1 )
	{
		pCmd = *ppBack;
		if ( !pCmd )
		{
			Com_Printf( "CmdSystem::RemoveCommand: %s not added\n", cmd_name );
			return;
		}
		if ( Q_stricmp( cmd_name, pCmd->pName ) == 0 )
		{
			*ppBack = pCmd->pNext;
			Mem_Free( pCmd );
			return;
		}
		ppBack = &pCmd->pNext;
	}
}

/*
========================
CommandExists
========================
*/
bool CommandExists( const char *cmd_name )
{
	for ( cmdFunction_t *cmd = cmd_functions; cmd; cmd = cmd->pNext )
	{
		if ( Q_stricmp( cmd_name, cmd->pName ) == 0 ) {
			return true;
		}
	}

	return false;
}

/*
========================
CompleteCommand
========================
*/
const char *CompleteCommand( const char *partial )
{
	strlen_t len = Q_strlen( partial );

	if ( !len ) {
		return nullptr;
	}

	cmdFunction_t *pCmd;
	cmdAlias_t *pAlias;

	// check for exact match
	for ( pCmd = cmd_functions; pCmd; pCmd = pCmd->pNext )
	{
		if ( Q_stricmp( partial, pCmd->pName ) == 0 ) {
			return pCmd->pName;
		}
	}
	for ( pAlias = cmd_alias; pAlias; pAlias = pAlias->pNext )
	{
		if ( Q_stricmp( partial, pAlias->name ) == 0 ) {
			return pAlias->name;
		}
	}

	// check for partial match
	for ( pCmd = cmd_functions; pCmd; pCmd = pCmd->pNext )
	{
		if ( Q_strnicmp( partial, pCmd->pName, len ) == 0 ) {
			return pCmd->pName;
		}
	}
	for ( pAlias = cmd_alias; pAlias; pAlias = pAlias->pNext )
	{
		if ( Q_strnicmp( partial, pAlias->name, len ) == 0 ) {
			return pAlias->name;
		}
	}

	return nullptr;
}

/*
========================
ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
========================
*/
void ExecuteString( char *text )
{
	TokenizeString( text, true );

	// execute the command line
	if ( !GetArgc() ) {
		// no tokens
		return;
	}

	// check functions
	for ( cmdFunction_t *pCmd = cmd_functions; pCmd; pCmd = pCmd->pNext )
	{
		if ( !Q_stricmp( cmd_argv[0], pCmd->pName ) )
		{
			if ( !pCmd->pFunction )
			{
				// forward to server command
				ExecuteString( va( "cmd %s", text ) );
			}
			else
			{
				pCmd->pFunction();
			}
			return;
		}
	}

	// check alias
	for ( cmdAlias_t *pAlias = cmd_alias; pAlias; pAlias = pAlias->pNext )
	{
		if ( !Q_stricmp( cmd_argv[0], pAlias->name ) )
		{
			if ( ++alias_count == ALIAS_LOOP_COUNT )
			{
				Com_Print( "ALIAS_LOOP_COUNT\n" );
				return;
			}
			CmdBuffer::InsertText( pAlias->pValue );
			return;
		}
	}
	
	// check cvars
	if ( Cvar_Command() ) {
		return;
	}

	// send it as a server command if we are connected
	Cmd_ForwardToServer();
}

/*
========================
Cmd_List_f
========================
*/
static void Cmd_List_f()
{
	int i = 0;
	for ( cmdFunction_t *pCmd = cmd_functions; pCmd; pCmd = pCmd->pNext, i++ )
	{
		Com_Printf( "%s\n", pCmd->pName );
	}
	Com_Printf( "%d commands\n", i );
}

/*
========================
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
========================
*/
static void Cmd_Wait_f()
{
	CmdBuffer::cmd_wait = true;
}

/*
========================
Cmd_Exec_f
========================
*/
static void Cmd_Exec_f()
{
	if ( GetArgc() != 2 )
	{
		Com_Print( "exec <filename> : execute a script file\n" );
		return;
	}

	const char *filename = GetArgv( 1 );

	char *f;
	FileSystem::LoadFile( filename, (void **)&f, 1 );
	if ( !f )
	{
		Com_Printf( "Couldn't exec %s\n", filename );
		return;
	}
	Com_Printf( "Execing %s\n", filename );

	CmdBuffer::InsertText( f );

	FileSystem::FreeFile( f );
}

/*
========================
Cmd_Echo_f

Just prints the rest of the line to the console
========================
*/
static void Cmd_Echo_f()
{
	for ( int i = 1; i < GetArgc(); ++i )
	{
		Com_Printf( "%s ", GetArgv( i ) );
	}
	Com_Print( "\n" );
}

/*
========================
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
========================
*/
static void Cmd_Alias_f()
{
	char cmd[1024];
	cmdAlias_t *pAlias;

	if ( GetArgc() == 1 )
	{
		Com_Print( "Current alias commands:\n" );
		for ( pAlias = cmd_alias; pAlias; pAlias = pAlias->pNext )
		{
			Com_Printf( "%s : %s\n", pAlias->name, pAlias->pValue );
		}
		return;
	}

	const char *aliasName = GetArgv( 1 );
	if ( strlen( aliasName ) >= MAX_ALIAS_NAME )
	{
		Com_Print( "Alias name is too long\n" );
		return;
	}

	// if the alias already exists, reuse it
	for ( pAlias = cmd_alias; pAlias; pAlias = pAlias->pNext )
	{
		if ( Q_stricmp( aliasName, pAlias->name ) == 0 )
		{
			Mem_Free( pAlias->pValue );
			break;
		}
	}

	if ( !pAlias )
	{
		pAlias = (cmdAlias_t*)Mem_ClearedAlloc( sizeof( cmdAlias_t ) );
		pAlias->pNext = cmd_alias;
		cmd_alias = pAlias;
	}
	strcpy( pAlias->name, aliasName );

	// copy the rest of the command line
	cmd[0] = 0;			// start out with a null string
	int c = GetArgc();
	for ( int i = 2; i < c; ++i )
	{
		strcat( cmd, GetArgv( i ) );
		if ( i != ( c - 1 ) )
		{
			strcat( cmd, " " );
		}
	}
	strcat( cmd, "\n" );

	pAlias->pValue = Mem_CopyString( cmd );
}

/*
========================
Cmd_Toggle_f

Toggles a convar as if it were a bool
TODO: This the wrong place for this? wtf why did I put it in the command system?
========================
*/
static void Cmd_Toggle_f()
{
	if ( GetArgc() != 2 )
	{
		Com_Print( "Bad parameters\n" );
		return;
	}

	const char *cmdName = GetArgv( 1 );
	cvar_t *var = Cvar_Find( cmdName );
	if ( !var )
	{
		Com_Printf( "Cvar \"%s\" does not exist\n", cmdName );
		return;
	}

	Cvar_SetBool( var, !var->GetBool() );

	Com_Printf( "Toggled %s to %s\n", var->GetName(), var->GetBool() ? "true" : "false" );
}

/*
========================
Init
========================
*/
void Init()
{
	CmdSystem::AddCommand( "cmdlist", Cmd_List_f, "Lists all available commands." );
	CmdSystem::AddCommand( "exec", Cmd_Exec_f, "Executes a script file." );
	CmdSystem::AddCommand( "echo", Cmd_Echo_f, "Prints arguments to the console." );
	CmdSystem::AddCommand( "alias", Cmd_Alias_f, "Creates a command alias." );
	CmdSystem::AddCommand( "toggle", Cmd_Toggle_f, "Toggles a convar as if it were a bool." );
	CmdSystem::AddCommand( "wait", Cmd_Wait_f, "Defers script execution until the next frame." );
}

/*
========================
Shutdown
========================
*/
void Shutdown()
{
	cmdFunction_t *cmd = cmd_functions;
	cmdFunction_t *lastCmd = nullptr;

	for ( ; cmd; cmd = cmd->pNext )
	{
		if ( lastCmd ) {
			Mem_Free( lastCmd );
		}

		lastCmd = cmd;
	}
	Mem_Free( lastCmd );

	cmdAlias_t *alias = cmd_alias;
	cmdAlias_t *lastAlias = nullptr;

	for ( ; alias; alias = alias->pNext )
	{
		if ( lastAlias ) {
			Mem_Free( lastAlias );
		}

		lastAlias = alias;

		Mem_Free( alias->pValue );
	}
	Mem_Free( lastAlias );

	for ( int i = 0; i < cmd_argc; ++i ) {
		Mem_Free( cmd_argv[i] );
	}
}

} // namespace CmdSystem
