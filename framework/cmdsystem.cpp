/*
===================================================================================================

	Quake script command processing module

	Provides command text buffering (Cbuf) and command execution (Cmd)
	Cmd names are case insensitive

===================================================================================================
*/

#include "framework_local.h"

#include "cmdsystem.h"

#define	MAX_CMD_BUFFER	16384
#define	MAX_CMD_LINE	1024

#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	80		// max tokens resulting from Cmd_TokenizeString

#define	MAX_ALIAS_NAME		32

struct cmdAlias_t
{
	char			name[MAX_ALIAS_NAME];
	char *			pValue;
	cmdAlias_t *	pNext;
};

// singly linked list of command aliases
static cmdAlias_t *cmd_alias;

struct cmd_t
{
	byte *	data;
	int		maxsize;
	int		cursize;
};

static int		cmd_wait;
static cmd_t	cmd_text;
static byte		cmd_text_buf[MAX_CMD_BUFFER];
static byte		defer_text_buf[MAX_CMD_BUFFER];

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
	if ( Cmd_Argc() == 2 ) {
		cmd_wait = Q_atoi( Cmd_Argv( 1 ) );
	} else {
		cmd_wait = 1;
	}
}

/*
===================================================================================================

	Command buffer

===================================================================================================
*/

/*
========================
Cbuf_Init
========================
*/
void Cbuf_Init()
{
	cmd_text.data = cmd_text_buf;
	cmd_text.maxsize = MAX_CMD_BUFFER;
	cmd_text.cursize = 0;
}

/*
========================
Cbuf_Shutdown
========================
*/
void Cbuf_Shutdown()
{
}

/*
========================
Cbuf_AddText

Adds command text at the end of the buffer
========================
*/
void Cbuf_AddText( const char *text )
{
	int l = static_cast<int>( strlen( text ) );

	if ( cmd_text.cursize + l >= cmd_text.maxsize )
	{
		Com_Print( "Cbuf_AddText: overflow\n" );
		return;
	}

	memcpy( &cmd_text.data[cmd_text.cursize], text, l );
	cmd_text.cursize += l;
}

/*
========================
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
========================
*/
void Cbuf_InsertText( const char *text )
{
	int len = static_cast<int>( strlen( text ) + 1 );
	if ( len + cmd_text.cursize > cmd_text.maxsize )
	{
		Com_Print( "Cbuf_InsertText overflowed\n" );
		return;
	}

	// move the existing command text
	// TODO: How can this be translated to memmove or something intrinsic?
#if 1
	for ( int i = cmd_text.cursize - 1; i >= 0; i-- )
	{
		cmd_text.data[i + len] = cmd_text.data[i];
	}
#else
	memmove( cmd_text.data + len, cmd_text.data, cmd_text.cursize - 1 );
#endif

	// copy the new text in
	memcpy( cmd_text.data, text, len - 1 );

	// add a \n
	cmd_text.data[len - 1] = '\n';

	cmd_text.cursize += len;
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
	char	line[MAX_CMD_LINE];
	int		quotes;

	alias_count = 0;		// don't allow infinite alias loops

	while ( cmd_text.cursize )
	{
		if ( cmd_wait ) {
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait--;
			break;
		}

		// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
		for ( i = 0; i < cmd_text.cursize; i++ )
		{
			if ( text[i] == '"' ) {
				quotes++;
			}
			if ( !( quotes & 1 ) && text[i] == ';' ) {
				// don't break if inside a quoted string
				break;
			}
			if ( text[i] == '\n' ) {
				break;
			}
		}

		if ( i >= ( MAX_CMD_LINE - 1 ) ) {
			i = MAX_CMD_LINE - 1;
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
	for ( int i = 1; i < argc; ++i )
	{
		if ( Q_strcmp( argv[i], "+set" ) == 0 && ( i + 2 ) < argc )
		{
			Cbuf_AddText( va( "set %s %s\n", argv[i + 1], argv[i + 2] ) );
			i += 2;
		}
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
	fsSize_t len = FileSystem::LoadFile( filename, (void **)&f, 1 );
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
	cmdAlias_t *pAlias;

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
===================================================================================================

	Command execution

===================================================================================================
*/

static int		cmd_argc;
static char *	cmd_argv[MAX_STRING_TOKENS];
static char		cmd_args[MAX_STRING_CHARS];

// possible commands to execute
cmdFunction_t *cmd_functions;

static void Cmd_Add( cmdFunction_t *pCmd )
{
	pCmd->pNext = cmd_functions;
	cmd_functions = pCmd;
}

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
		if ( Q_stricmp( cmd_name, pCmd->pName ) == 0 )
		{
			Com_Printf( "Cmd_AddCommand: %s already defined as %s\n", cmd_name, pCmd->pName );
			return;
		}
	}

	pCmd = (cmdFunction_t *)Mem_Alloc( sizeof( cmdFunction_t ) );
	pCmd->pName = cmd_name;
	pCmd->pHelp = help;
	pCmd->pFunction = function;

	pCmd->flags = 0;

	Cmd_Add( pCmd );
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
Cmd_Exists
========================
*/
bool Cmd_Exists( const char *cmd_name )
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
Cmd_ExecuteCvar

Handles variable inspection and changing from the console

called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
command.  Returns true if the command was a variable reference that
was handled. (print or change)
========================
*/
static bool Cmd_ExecuteCvar()
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

	// set it!
	Cvar_FindSetString( var->name.c_str(), Cmd_Argv( 1 ) );

	return true;
}

/*
========================
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
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
		if ( !Q_stricmp( cmd_argv[0], pCmd->pName ) )
		{
			if ( !pCmd->pFunction )
			{
				// forward to server command
#ifdef Q_ENGINE
				Cmd_ExecuteString( va( "cmd %s", text ) );
#endif
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
			Cbuf_InsertText( pAlias->pValue );
			return;
		}
	}

	// check cvars
	if ( Cmd_ExecuteCvar() ) {
		return;
	}

	// send it as a server command if we are connected
#ifdef Q_ENGINE
	Cmd_ForwardToServer();
#else
	Com_Printf( "Unknown command \"%s\"\n", text );
#endif
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
	Cmd_AddCommand( "wait", Cmd_Wait_f, "Defers script execution until the next frame." );
}

/*
========================
Cmd_Shutdown
========================
*/
void Cmd_Shutdown()
{
	// Clean up functions
	while ( cmd_functions )
	{
		cmdFunction_t *pNext = cmd_functions->pNext;
		if ( !( cmd_functions->flags & CMD_STATIC ) )
		{
			Mem_Free( cmd_functions );
		}
		cmd_functions = pNext;
	}

	// Clean up aliases
	while ( cmd_alias )
	{
		cmdAlias_t *pNext = cmd_alias->pNext;
		Mem_Free( cmd_alias->pValue );
		Mem_Free( cmd_alias );
		cmd_alias = pNext;
	}

	// Clean up the argc
	for ( int i = 0; i < cmd_argc; ++i ) {
		Mem_Free( cmd_argv[i] );
		cmd_argv[i] = nullptr;
	}
	cmd_argc = 0;
	cmd_args[0] = 0;
}

/*
===================================================================================================

	Statically defined commands

===================================================================================================
*/

StaticCmd::StaticCmd( const char *Name, xcommand_t Function, const char *Help, uint32 Flags )
{
	pName = Name;
	pHelp = Help;
	pFunction = Function;
	pNext = nullptr;

	flags = Flags |= CMD_STATIC;

	Cmd_Add( this );
}
