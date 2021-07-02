/*
===================================================================================================

	ImGui console and notify area

	FIXME: you can only browse history after you've submitted something, this is due to the
	placement of con.completionPopup after Con2_Submit

===================================================================================================
*/

#include "cl_local.h"

#include <vector>
#include <string>
#include <array>
#include <algorithm>

#include "imgui.h"
#include "q_imgui_imp.h"

namespace UI::Console
{

static constexpr int MAX_MATCHES	= 9;	// 10th match is "..."
static constexpr int MAX_NOTIFIES	= 8;	// should be a cvar

static constexpr uint32 CmdColor	= colors::green;

static cvar_t *con_notifytime;
static cvar_t *con_drawnotify;
static cvar_t *con_allownotify;

struct notify_t
{
	char		message[256];
	float		timeLeft;
};

struct editLine_t
{
	char data[256];

	void Clear()
	{
		memset( data, 0, sizeof( data ) );
	}
};

// TODO: Experiment with a circular buffer for the history lines and main buffer
struct console2_t
{
	// the main console buffer, infinitely expanding (bad?)
	std::string				buffer;

	// our input line, and infinitely expanding history (bad?)
	editLine_t				editLine;
	std::vector<editLine_t>	historyLines;
	int						historyPosition = -1;

	// alphabetically sorted lists of matching commands and cvars
	std::vector<std::string_view>	entryMatches;
	int						beginCvars = 0;				// contains the offset to the cvars in the matches list, so we can sort seperately
	int						completionPosition = -1;

	notify_t				notifies[MAX_NOTIFIES];		// cls.realtime time the line was generated, for transparent notify lines
	int						currentNotify;

	bool					scrollToBottom;
	bool					completionPopup;
	bool					wordWrap;
	bool					ignoreEdit;					// when true, CallbackEdit will be ignored once
	bool					initialized;

	// so we can use the console instantly from main
	console2_t()
	{
		// reserve 16384 characters by default
		buffer.reserve( 16384 );
		historyLines.reserve( 32 );

		entryMatches.reserve( 64 );
	}

};

static console2_t con;

//=================================================================================================

static bool CanAddNotifies()
{
	if ( !con.initialized ) {
		return false;
	}

	// TEMP
	/*if ( !developer->GetBool() ) {
		return false;
	}*/

	if ( !con_allownotify->GetBool() ) {
		return false;
	}

	return true;
}

static bool CanDrawNotifies()
{
	// TEMP
	/*if ( !developer->GetBool() ) {
		return false;
	}*/

	if ( !con_drawnotify->GetBool() ) {
		return false;
	}

	return true;
}

void Print( const char *txt )
{
	con.buffer.append( txt );

	if ( !CanAddNotifies() ) {
		return;
	}

	notify_t &notify = con.notifies[con.currentNotify & ( MAX_NOTIFIES - 1 )];

	Q_strcpy_s( notify.message, txt );
	notify.timeLeft = con_notifytime->GetFloat();

	++con.currentNotify;
}

static int TextEditCallback( ImGuiInputTextCallbackData *data )
{
	switch ( data->EventFlag )
	{
	case ImGuiInputTextFlags_CallbackEdit:
	{
		if ( !con.ignoreEdit && data->BufTextLen != 0 )
		{
			con.completionPosition = -1;
			con.completionPopup = true;
		}
		con.ignoreEdit = false;
	}
	break;
	case ImGuiInputTextFlags_CallbackCompletion:
	{
		const char *cmd = Cmd_CompleteCommand( data->Buf );
		if ( !cmd ) {
			cmd = Cvar_CompleteVariable( data->Buf );
		}

		if ( cmd )
		{
			// replace entire buffer
			data->DeleteChars( 0, data->BufTextLen );
			data->InsertChars( data->CursorPos, cmd );
		}
	}
	break;
	case ImGuiInputTextFlags_CallbackHistory:
	{
		if ( con.completionPopup )
		{
			con.ignoreEdit = true;

			switch ( data->EventKey )
			{
			case ImGuiKey_UpArrow:
				if ( con.completionPosition == -1 ) {
					// Start at the end
					con.completionPosition = (int64)con.entryMatches.size() - 1;
				}
				else if ( con.completionPosition > 0 ) {
					// Decrement
					--con.completionPosition;
				}
				break;
			case ImGuiKey_DownArrow:
				if ( con.completionPosition < (int64)con.entryMatches.size() ) {
					++con.completionPosition;
				}
				if ( con.completionPosition == (int64)con.entryMatches.size() ) {
					--con.completionPosition;
				}
				break;
			}

			const char *match_str = con.completionPosition >= 0 ? con.entryMatches[con.completionPosition].data() : "";
			data->DeleteChars( 0, data->BufTextLen );
			data->InsertChars( 0, match_str );
		}
		else if ( !con.historyLines.empty() )
		{
			switch ( data->EventKey )
			{
			case ImGuiKey_UpArrow:
				if ( con.historyPosition == -1 ) {
					// Start at the end
					con.historyPosition = (int64)con.historyLines.size() - 1;
				}
				else if ( con.historyPosition > 0 ) {
					// Decrement
					--con.historyPosition;
				}
				break;
			case ImGuiKey_DownArrow:
				if ( con.historyPosition < (int64)con.historyLines.size() ) {
					++con.historyPosition;
				}
				if ( con.historyPosition == (int64)con.historyLines.size() ) {
					--con.historyPosition;
				}
				break;
			}

			const char *history_str = con.historyPosition >= 0 ? con.historyLines[con.historyPosition].data : "";
			data->DeleteChars( 0, data->BufTextLen );
			data->InsertChars( 0, history_str );
		}
	}
	break;
	}

	return 0;
}

static void Clear_f()
{
	con.buffer.clear();
}

// this is the wrong place for this really, oh well
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
		if ( listAll || strstr( pCmd->pName, searchName ) )					// TODO: need Q_ version
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

	std::sort( cmdMatches.begin(), cmdMatches.end() );

	std::vector<cvar_t *> cvarMatches;

	// find cvar matches
	for ( cvar_t *pVar = cvar_vars; pVar; pVar = pVar->pNext )
	{
		if ( listAll || strstr( pVar->GetName(), searchName ) )			// TODO: need Q_ version
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

	std::sort( cvarMatches.begin(), cvarMatches.end() );

	// build the format string

	char formatString[128];
	Q_sprintf_s( formatString, "%%-%us" "%%-%us" "%%-%us\n", longestName, longestValue, longestHelp );

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

	for ( const auto &pCmd : cmdMatches )
	{
		cmdNameGreen = S_COLOR_GREEN;
		cmdNameGreen.append( pCmd->pName );
		Com_Printf( formatString, cmdNameGreen.c_str(), S_COLOR_WHITE "", pCmd->pHelp ? pCmd->pHelp : "" );
	}

	for ( const auto &pVar : cvarMatches )
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
		if ( Q_strcmp( pCmd->pName, name ) == 0 )
		{
			Com_Printf( " - %s\n", pCmd->pHelp );
		}
	}

	// search cvars
	for ( cvar_t *pVar = cvar_vars; pVar; pVar = pVar->pNext )
	{
		if ( Q_strcmp( pVar->GetName(), name ) == 0 )
		{
			Com_Printf( " - %s\n", pVar->help.c_str() );
		}
	}
}

void Init()
{
	con_notifytime = Cvar_Get( "con_notifytime", "8", 0, "Time in seconds that notifies are visible before expiring." );
	con_drawnotify = Cvar_Get( "con_drawnotify", "1", 0, "If true, notifies can be drawn." );
	con_allownotify = Cvar_Get( "con_allownotify", "1", 0, "If true, notifies can be posted." );

	Cmd_AddCommand( "clear", Clear_f, "Clears the console buffer." );
	Cmd_AddCommand( "find", Find_f, "Finds all cmds and cvars with <param> in the name, use * to list all." );
	Cmd_AddCommand( "help", Help_f, "Displays help for a given cmd or cvar." );

	con.initialized = true;
}

/*
========================
Submit
Public

Submits the text stored in con.editLine, then clears it
========================
*/
static void Submit()
{
	con.completionPopup = false;
	con.historyPosition = -1;

	// don't push back if the last history content was this very command
	// (mimics cmd)
	if ( con.historyLines.empty() || Q_strcmp( con.historyLines.back().data, con.editLine.data ) != 0 )
	{
		con.historyLines.push_back( con.editLine );
	}

	con.scrollToBottom = true;

	// backslash text are commands, else chat
	if ( con.editLine.data[0] == '\\' || con.editLine.data[0] == '/' )
	{
		// skip the >
		Cbuf_AddText( con.editLine.data + 1 );
	}
	else
	{
		// valid command
		Cbuf_AddText( con.editLine.data );
	}

	Cbuf_AddText( "\n" );

	Com_Printf( "] %s\n", con.editLine.data );

	con.editLine.Clear();
}

int StringSort( const void *p1, const void *p2 )
{
	return Q_strcmp( (const char *)p1, (const char *)p2 );
}

/*
========================
RegenerateMatches

Finds all matches for a given partially complete string
========================
*/
static void RegenerateMatches( const char *partial )
{
	ASSUME( partial );

	con.entryMatches.clear();

	if ( !partial[0] )
	{
		return;
	}

	strlen_t partialLength = Q_strlen( partial );

	// find command matches
	for ( cmdFunction_t *pCmd = cmd_functions; pCmd; pCmd = pCmd->pNext )
	{
		if ( Q_strncmp( partial, pCmd->pName, partialLength ) == 0 )
		{
			con.entryMatches.push_back( pCmd->pName );
		}
	}

	std::sort( con.entryMatches.begin(), con.entryMatches.end() );

	// index to the first cvar
	size_t beginCvars = con.entryMatches.size();

	// find cvar matches
	for ( cvar_t *pVar = cvar_vars; pVar; pVar = pVar->pNext )
	{
		if ( Q_strncmp( partial, pVar->GetName(), partialLength ) == 0 )
		{
			con.entryMatches.push_back( pVar->GetName() );
		}
	}

	std::sort( con.entryMatches.begin() + beginCvars, con.entryMatches.end() );

	// give con our variable
	con.beginCvars = static_cast<int>( beginCvars );
}

/*
========================
ShowConsole

Draws the console using ImGui
========================
*/
void ShowConsole( bool *pOpen )
{
	assert( *pOpen );

	ImGui::PushStyleVar( ImGuiStyleVar_WindowMinSize, ImVec2( 330.0f, 250.0f ) );

	ImGui::SetNextWindowSize( ImVec2( 600.0f, 800.0f ), ImGuiCond_FirstUseEver );
	if ( !ImGui::Begin( "Console", pOpen ) )
	{
		ImGui::PopStyleVar();
		ImGui::End();
		return;
	}

	ImGui::PopStyleVar();

	if ( ImGui::BeginPopupContextItem() )
	{
		if ( ImGui::MenuItem( "Close Console" ) )
		{
			*pOpen = false;
		}
		ImGui::EndPopup();
	}

	// save the old wordwrap state
	bool wordWrap = con.wordWrap;

	ImGuiWindowFlags scrollFlags = wordWrap ? 0 : ImGuiWindowFlags_HorizontalScrollbar;

	// Reserve enough left-over height for 1 separator + 1 input text
	ImGui::BeginChild( "ScrollingRegion", ImVec2( 0.0f, -ImGui::GetFrameHeightWithSpacing() ), true, scrollFlags );

	if ( ImGui::BeginPopupContextWindow() )
	{
		ImGui::Checkbox( "Word wrap", &con.wordWrap );
		ImGui::EndPopup();
	}

	if ( wordWrap )
	{
		ImGui::PushTextWrapPos( 0.0f );
	}

	if ( !con.buffer.empty() )
	{
		const char *bufferStart = con.buffer.c_str();
		const char *bufferEnd = con.buffer.c_str() + con.buffer.size() - 1;

		const int bufferSize = (int)con.buffer.size() - 1; // skip nul

		if ( bufferSize > 16384 )
		{
			// draw the buffer in 16384 chunks so we can avoid over-running the max vertex count of 65536
			// (16384 * 4 == 65536)
			const char *newStart = bufferStart;

			int chunkSize = 16384;
			int amountLeft = bufferSize;

			ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0.0f, 0.0f ) );

			while ( true )
			{
				// starting from the end of the chunk we're supposed to be drawing now, step back to find a newline
				// then use that as the end instead, and subsequently the start of the next chunk
				// this goes under the assumption that there's a newline somewhere... With more than 16384 chars
				// in the buffer this will *always* be true

				const char *newEnd = newStart + chunkSize;

				for ( const char *i = newEnd; i > newStart; --i )
				{
					if ( *i == '\n' )
					{
						chunkSize -= ( chunkSize - static_cast<int>( i - newStart ) ) - 1;
						newEnd = i;
						break;
					}
				}

				ImGui::TextUnformatted( newStart, newEnd );

				amountLeft -= chunkSize;
				newStart += chunkSize;

				if ( amountLeft <= 0 )
				{
					break;
				}

				if ( amountLeft < 16384 )
				{
					chunkSize = amountLeft;
				}
			}

			ImGui::PopStyleVar();
		}
		else
		{
			ImGui::TextUnformatted( bufferStart, bufferEnd );
		}
	}

	if ( wordWrap )
	{
		ImGui::PopTextWrapPos();
	}

	if ( con.scrollToBottom || ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
	{
		ImGui::SetScrollHereY( 1.0f );
	}

	con.scrollToBottom = false;

	ImGui::EndChild();

	// input line
	const ImGuiInputTextFlags inputFlags =
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion |
		ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_CallbackAlways;

	// steal focus from the window upon opening, no matter what
	bool focusOnInput = ImGui::IsWindowAppearing() ? true : false;

	ImGui::PushItemWidth( ImGui::GetWindowContentRegionWidth() - 64.0f );

	if ( ImGui::InputText( "##Input", con.editLine.data, sizeof( con.editLine.data ), inputFlags, TextEditCallback, nullptr ) )
	{
		Submit();
		focusOnInput = true;
	}

	ImGui::PopItemWidth();

	// set the input text as the default item
	ImGui::SetItemDefaultFocus();

	// store the bottom left position of the input window, then add the spacing X Y
	const ImVec2 popupPosition( ImGui::GetItemRectMin().x - ImGui::GetStyle().ItemSpacing.x, ImGui::GetItemRectMax().y + ImGui::GetStyle().ItemSpacing.y + 5.0f );

	ImGui::SameLine();

	if ( ImGui::Button( "Submit" ) )
	{
		Submit();
		focusOnInput = true;
	}

	if ( focusOnInput )
	{
		ImGui::SetKeyboardFocusHere( 2 );
	}

	if ( con.completionPopup )
	{
		const ImGuiWindowFlags popFlags =
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize;

		if ( !con.ignoreEdit )
		{
			RegenerateMatches( con.editLine.data );
		}

		const uint totalMatches = static_cast<uint>( con.entryMatches.size() );

		if ( totalMatches != 0 )
		{
			ImGui::SetNextWindowPos( popupPosition );

			// the command popup should only list the first n entries, starting with commands
			if ( ImGui::Begin( "CommandPopup", nullptr, popFlags ) )
			{
				int i = 0;

				// loop for the cmds, they're printed green
				ImGui::PushStyleColor( ImGuiCol_Text, CmdColor );
				for ( ; i < con.beginCvars && i < MAX_MATCHES; ++i )
				{
					ImGui::Selectable( con.entryMatches[i].data() );
				}
				ImGui::PopStyleColor();

				// loop for the cvars
				for ( ; i < (int)con.entryMatches.size() && i < MAX_MATCHES; ++i )
				{
					ImGui::Selectable( con.entryMatches[i].data() );
				}

				if ( i >= MAX_MATCHES )
				{
					ImGui::Selectable( "...", false, ImGuiSelectableFlags_Disabled );
				}
			}

			ImGui::End();
		}
	}
	/*else
	{
		// TODO: show history?
	}*/

	ImGui::End();
}

/*
===============================================================================

	Notify

===============================================================================
*/

void ShowNotify()
{
	if ( !CanDrawNotifies() )
	{
		return;
	}

	const int currentNotify = con.currentNotify;
	const int minNotify = con.currentNotify - MAX_NOTIFIES + 1;

	// check to see if we have some stuff to display, to avoid the window begin call
	bool show = false;
	for ( int i = minNotify; i < currentNotify; ++i )
	{
		const int index = i & ( MAX_NOTIFIES - 1 );

		if ( con.notifies[index].timeLeft > 0.0f )
		{
			show = true;
			break;
		}
	}
	if ( !show )
	{
		return;
	}

	const ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove;

	ImGui::SetNextWindowPos( ImVec2( 0.0f, 0.0f ), ImGuiCond_Always );
	ImGui::SetNextWindowBgAlpha( 0.25f );

	ImGui::Begin( "Notify Area", nullptr, windowFlags );

	ImGuiIO &io = ImGui::GetIO();

	for ( int i = minNotify; i < currentNotify; ++i )
	{
		const int index = i & ( MAX_NOTIFIES - 1 );

		notify_t &notify = con.notifies[index];

		if ( notify.timeLeft <= 0.0f )
		{
			continue;
		}

		uint32 alpha = 255;

		if ( notify.timeLeft <= 0.5f )
		{
			float fltAlpha = Clamp( notify.timeLeft, 0.0f, 0.5f ) / 0.5f;

			alpha = static_cast<uint32>( fltAlpha * 255.0f );

			if ( fltAlpha < 0.2f )
			{
				ImGui::SetCursorPosY( ImGui::GetCursorPosY() - ImGui::GetFont()->FontSize * ( 1.0f - fltAlpha / 0.2f ) );
			}
		}

		notify.timeLeft -= cls.frametime;

		const uint32 packedColor = PackColor( 255, 255, 255, alpha );
		ImGui::PushStyleColor( ImGuiCol_Text, packedColor );
		ImGui::TextUnformatted( notify.message );
		ImGui::PopStyleColor();
	}

	ImGui::End();
}

void ClearNotify()
{
	for ( uint i = 0; i < MAX_NOTIFIES; ++i )
	{
		con.notifies[i].timeLeft = 0.0f;
	}
}

} // namespace ui::console
