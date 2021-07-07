/*
===================================================================================================

	Quake script command processing module

	Provides command text buffering (Cbuf) and command execution (Cmd)

===================================================================================================
*/

#include "engine.h"

#include "cmd.h"

#define	MAX_ALIAS_NAME	32

struct cmdalias_t
{
	char			name[MAX_ALIAS_NAME];
	char *			pValue;
	cmdalias_t *	pNext;
};

// singly linked list of command aliases
static cmdalias_t *cmd_alias;

static bool cmd_wait;

#define	ALIAS_LOOP_COUNT	16
static int alias_count;		// for detecting runaway loops

static char cmd_null_string[1]; // lame

//=================================================================================================

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
	cmd_wait = true;
}

/*
===================================================================================================

	Command buffer

===================================================================================================
*/

sizebuf_t	cmd_text;
byte		cmd_text_buf[8192];

byte		defer_text_buf[8192];

/*
========================
Cbuf_Init
========================
*/
void Cbuf_Init()
{
	SZ_Init( &cmd_text, cmd_text_buf, sizeof( cmd_text_buf ) );
}

/*
========================
Cbuf_Shutdown
========================
*/
void Cbuf_Shutdown()
{
	SZ_Init( &cmd_text, cmd_text_buf, sizeof( cmd_text_buf ) );
}

/*
========================
Cbuf_AddText

Adds command text at the end of the buffer
========================
*/
void Cbuf_AddText( const char *text )
{
	int l = (int)strlen( text );

	if ( cmd_text.cursize + l >= cmd_text.maxsize )
	{
		Com_Print( "Cbuf_AddText: overflow\n" );
		return;
	}

	SZ_Write( &cmd_text, text, l );
}

/*
========================
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
========================
*/
void Cbuf_InsertText( const char *text )
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
	Cbuf_AddText( text );

	// add the copied off data
	if ( templen )
	{
		SZ_Write( &cmd_text, temp, templen );
	}
}

/*
========================
Cbuf_CopyToDefer
========================
*/
void Cbuf_CopyToDefer()
{
	memcpy( defer_text_buf, cmd_text_buf, cmd_text.cursize );
	defer_text_buf[cmd_text.cursize] = '\0';
	cmd_text.cursize = '\0';
}

/*
========================
Cbuf_InsertFromDefer
========================
*/
void Cbuf_InsertFromDefer()
{
	Cbuf_InsertText( (char*)defer_text_buf );
	defer_text_buf[0] = '\0';
}

/*
========================
Cbuf_Execute
========================
*/
void Cbuf_Execute()
{
	int		i;
	char	*text;
	char	line[1024];
	int		quotes;

	alias_count = 0;		// don't allow infinite alias loops

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
		Cmd_ExecuteString( line );

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
Cbuf_AddEarlyCommands

Adds command line parameters as script statements
Commands lead with a +, and continue until another +

Set commands are added early, so they are guaranteed to be set before
the client and server initialize for the first time.

Other commands are added late, after all initialization is complete.
========================
*/
void Cbuf_AddEarlyCommands( int argc, char **argv )
{
	for ( int i = 0; i < argc; ++i )
	{
		if ( Q_strcmp( argv[i], "+set" ) != 0 ) {
			continue;
		}
		Cbuf_AddText( va( "set %s %s\n", argv[i + 1], argv[i + 2] ) );
		i += 2;
	}
}

/*
========================
Cbuf_AddLateCommands

Adds command line parameters as script statements
Commands lead with a + and continue until another + or -
quake +vid_ref gl +map amlev1

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
========================
*/
bool Cbuf_AddLateCommands( int argc, char **argv )
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
		Cbuf_AddText( build );
	}
	
	return ret;
}

/*
===================================================================================================

	Script commands

===================================================================================================
*/

/*
========================
Cmd_Exec_f
========================
*/
static void Cmd_Exec_f()
{
	if ( Cmd_Argc() != 2 )
	{
		Com_Print( "exec <filename> : execute a script file\n" );
		return;
	}

	const char *filename = Cmd_Argv( 1 );

	char *f;
	int len = FileSystem::LoadFile( filename, (void **)&f, 1 );
	if ( !f )
	{
		Com_Printf( "Couldn't exec %s\n", filename );
		return;
	}
	Com_Printf( "Execing %s\n", filename );

	Cbuf_InsertText( f );

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
	for ( int i = 1; i < Cmd_Argc(); ++i )
	{
		Com_Printf( "%s ", Cmd_Argv( i ) );
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
	cmdalias_t *pAlias;

	if ( Cmd_Argc() == 1 )
	{
		Com_Print( "Current alias commands:\n" );
		for ( pAlias = cmd_alias; pAlias; pAlias = pAlias->pNext )
		{
			Com_Printf( "%s : %s\n", pAlias->name, pAlias->pValue );
		}
		return;
	}

	const char *aliasName = Cmd_Argv( 1 );
	if ( strlen( aliasName ) >= MAX_ALIAS_NAME )
	{
		Com_Print( "Alias name is too long\n" );
		return;
	}

	// if the alias already exists, reuse it
	for ( pAlias = cmd_alias; pAlias; pAlias = pAlias->pNext )
	{
		if ( Q_strcmp( aliasName, pAlias->name ) == 0 )
		{
			Mem_Free( pAlias->pValue );
			break;
		}
	}

	if ( !pAlias )
	{
		pAlias = (cmdalias_t*)Mem_ClearedAlloc( sizeof( cmdalias_t ) );
		pAlias->pNext = cmd_alias;
		cmd_alias = pAlias;
	}
	strcpy( pAlias->name, aliasName );

	// copy the rest of the command line
	cmd[0] = 0;			// start out with a null string
	int c = Cmd_Argc();
	for ( int i = 2; i < c; ++i )
	{
		strcat( cmd, Cmd_Argv( i ) );
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

/*
===================================================================================================

	Command execution

===================================================================================================
*/

static int		cmd_argc;
static char *	cmd_argv[MAX_STRING_TOKENS];
static char		cmd_args[MAX_STRING_CHARS];

// possible commands to execute
cmdFunction_t *cmd_functions;

/*
========================
Cmd_Argc
========================
*/
int Cmd_Argc()
{
	return cmd_argc;
}

/*
========================
Cmd_Argv
========================
*/
char *Cmd_Argv( int arg )
{
	assert( arg >= 0 && arg < cmd_argc );
	if ( arg >= cmd_argc ) {
		return cmd_null_string;
	}
	return cmd_argv[arg];
}

/*
========================
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
========================
*/
char *Cmd_Args()
{
	return cmd_args;
}

/*
========================
Cmd_MacroExpandString
========================
*/
static char *Cmd_MacroExpandString( char *text )
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
Cmd_TokenizeString

Parses the given string into command line tokens.
$Cvars will be expanded unless they are in a quoted token
========================
*/
void Cmd_TokenizeString( char *text, bool macroExpand )
{
	// clear the args from the last string
	for ( int i = 0; i < cmd_argc; i++ ) {
		Mem_Free( cmd_argv[i] );
	}

	cmd_argc = 0;
	cmd_args[0] = 0;

	// macro expand the text
	if ( macroExpand ) {
		text = Cmd_MacroExpandString( text );
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
				} else {
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
Cmd_AddCommand
========================
*/
void Cmd_AddCommand( const char *cmd_name, xcommand_t function, const char *help /*= nullptr*/ )
{
	// fail if the command is a variable name
	if ( Cvar_Find( cmd_name ) )
	{
		Com_Printf( "Cmd_AddCommand: %s already defined as a var\n", cmd_name );
		return;
	}

	cmdFunction_t *pCmd;

	// fail if the command already exists
	for ( pCmd = cmd_functions; pCmd; pCmd = pCmd->pNext )
	{
		if ( Q_strcmp( cmd_name, pCmd->pName ) == 0 )
		{
			Com_Printf( "Cmd_AddCommand: %s already defined\n", cmd_name );
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
Cmd_RemoveCommand
========================
*/
void Cmd_RemoveCommand( const char *cmd_name )
{
	cmdFunction_t *pCmd, **ppBack;

	ppBack = &cmd_functions;
	while ( 1 )
	{
		pCmd = *ppBack;
		if ( !pCmd )
		{
			Com_Printf( "Cmd_RemoveCommand: %s not added\n", cmd_name );
			return;
		}
		if ( Q_strcmp( cmd_name, pCmd->pName ) == 0 )
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
Cmd_Exists
========================
*/
bool Cmd_Exists( const char *cmd_name )
{
	for ( cmdFunction_t *cmd = cmd_functions; cmd; cmd = cmd->pNext )
	{
		if ( Q_strcmp( cmd_name, cmd->pName ) == 0 ) {
			return true;
		}
	}

	return false;
}

/*
========================
Cmd_CompleteCommand
========================
*/
const char *Cmd_CompleteCommand( const char *partial )
{
	strlen_t len = Q_strlen( partial );

	if ( !len ) {
		return nullptr;
	}

	cmdFunction_t *pCmd;
	cmdalias_t *pAlias;

	// check for exact match
	for ( pCmd = cmd_functions; pCmd; pCmd = pCmd->pNext )
	{
		if ( Q_strcmp( partial, pCmd->pName ) == 0 ) {
			return pCmd->pName;
		}
	}
	for ( pAlias = cmd_alias; pAlias; pAlias = pAlias->pNext )
	{
		if ( Q_strcmp( partial, pAlias->name ) == 0 ) {
			return pAlias->name;
		}
	}

	// check for partial match
	for ( pCmd = cmd_functions; pCmd; pCmd = pCmd->pNext )
	{
		if ( Q_strncmp( partial, pCmd->pName, len ) == 0 ) {
			return pCmd->pName;
		}
	}
	for ( pAlias = cmd_alias; pAlias; pAlias = pAlias->pNext )
	{
		if ( Q_strncmp( partial, pAlias->name, len ) == 0 ) {
			return pAlias->name;
		}
	}

	return nullptr;
}

/*
========================
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
========================
*/
void Cmd_ExecuteString( char *text )
{
	Cmd_TokenizeString( text, true );

	// execute the command line
	if ( !Cmd_Argc() ) {
		// no tokens
		return;
	}

	// check functions
	for ( cmdFunction_t *pCmd = cmd_functions; pCmd; pCmd = pCmd->pNext )
	{
		if ( !Q_strcasecmp( cmd_argv[0], pCmd->pName ) )
		{
			if ( !pCmd->pFunction )
			{
				// forward to server command
				Cmd_ExecuteString( va( "cmd %s", text ) );
			}
			else
			{
				pCmd->pFunction();
			}
			return;
		}
	}

	// check alias
	for ( cmdalias_t *pAlias = cmd_alias; pAlias; pAlias = pAlias->pNext )
	{
		if ( !Q_strcasecmp( cmd_argv[0], pAlias->name ) )
		{
			if ( ++alias_count == ALIAS_LOOP_COUNT )
			{
				Com_Print( "ALIAS_LOOP_COUNT\n" );
				return;
			}
			Cbuf_InsertText( pAlias->pValue );
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
Cmd_Init
========================
*/
void Cmd_Init()
{
	Cmd_AddCommand( "cmdlist", Cmd_List_f, "Lists all available commands." );
	Cmd_AddCommand( "exec", Cmd_Exec_f, "Executes a script file." );
	Cmd_AddCommand( "echo", Cmd_Echo_f, "Prints arguments to the console." );
	Cmd_AddCommand( "alias", Cmd_Alias_f, "Creates a command alias." );
	Cmd_AddCommand( "toggle", Cmd_Toggle_f, "Toggles a convar as if it were a bool." );
	Cmd_AddCommand( "wait", Cmd_Wait_f, "Defers script execution until the next frame." );
}

/*
========================
Cmd_Shutdown
========================
*/
void Cmd_Shutdown()
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

	cmdalias_t *alias = cmd_alias;
	cmdalias_t *lastAlias = nullptr;

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
