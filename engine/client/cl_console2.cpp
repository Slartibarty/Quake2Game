/*
===================================================================================================

	ImGui console

===================================================================================================
*/

#include "cl_local.h"

#include <vector>
#include <string>

#include "imgui.h"

static cvar_t *con_notifytime;

struct editLine_t
{
	char data[256];

	void Clear()
	{
		memset( data, 0, sizeof( data ) );
	}
};

struct console2_t
{
	std::string				buffer;					// TODO: this is going to be painfully slow to clear since it memsets the entire buffer...

	editLine_t				editLine;
	std::vector<editLine_t>	historyLines;
	int						historyPosition = -1;

	bool					scrollToBottom;

	// so we can use the console instantly from main
	console2_t()
	{
		// reserve 32768 chars by default
		buffer.reserve( 32768 );
		historyLines.reserve( 32 );
	}

};

static console2_t con;

//=================================================================================================

void Con2_Print( const char *txt )
{
	con.buffer.append( txt );
}

static int Con2_TextEditCallback( ImGuiInputTextCallbackData *data )
{
	switch ( data->EventFlag )
	{
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
		if ( !con.historyLines.empty() )
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

// clears the edit line
static void Con2_Clear_f()
{
	con.buffer.clear();
}

void Con2_Init()
{
	con_notifytime = Cvar_Get( "con_notifytime", "3", 0 );

	Cmd_AddCommand( "clear", Con2_Clear_f );
}

/*
========================
Con2_Submit
Public

Submits the text stored in con.editLine, then clears it
========================
*/
static void Con2_Submit()
{
	con.historyLines.push_back( con.editLine );

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

/*
========================
Con2_ShowConsole

Draws the console using ImGui
========================
*/
void Con2_ShowConsole( bool *pOpen )
{
	//ImGui::SetNextWindowPos( ImVec2( 40, 40 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 560, 400 ), ImGuiCond_FirstUseEver );
	if ( !ImGui::Begin( "Console", pOpen ) )
	{
		ImGui::End();
		return;
	}

	// Reserve enough left-over height for 1 separator + 1 input text
	const float scrollWidth = ImGui::GetWindowContentRegionWidth();
	const float scrollHeight = -( ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.y );
	ImGui::BeginChild( "ScrollingRegion", ImVec2( 0, scrollHeight ), true );

	ImGui::TextUnformatted( con.buffer.c_str() );

	if ( con.scrollToBottom || ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
	{
		ImGui::SetScrollHereY( 1.0f );
	}

	con.scrollToBottom = false;

	ImGui::EndChild();

	// input line
	bool reclaimFocus = false;
	ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;

	if ( ImGui::InputText( "", con.editLine.data, sizeof( con.editLine.data ), inputFlags, Con2_TextEditCallback, nullptr ) )
	{
		Con2_Submit();

		reclaimFocus = true;
	}

	ImGui::SameLine();

	// auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if ( reclaimFocus ) {
		ImGui::SetKeyboardFocusHere( -1 ); // Auto focus previous widget
	}

	ImGui::End();
}

void Con2_ShowNotify() {}
void Con2_ClearNotify() {}
