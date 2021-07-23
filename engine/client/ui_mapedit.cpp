/*
===================================================================================================

	ImGui map editor

===================================================================================================
*/

#include "cl_local.h"

#include <vector>
#include <string>

#include "imgui.h"

namespace UI::MapEdit
{

static struct mapEdit_t
{
	bool ui_about;
} mapEdit;

static void ShowMenuBar_NoDocument( bool *pOpen )
{
	if ( ImGui::BeginMenuBar() )
	{
		if ( ImGui::BeginMenu( "File" ) )
		{
			ImGui::MenuItem( "New", "Ctrl+N" );
			ImGui::MenuItem( "Open...", "Ctrl+O" );
			ImGui::Separator();
			if ( ImGui::MenuItem( "Exit", "Ctrl+Q" ) )
			{
				*pOpen = false;
			}

			ImGui::EndMenu();
		}
		if ( ImGui::BeginMenu( "Tools" ) )
		{
			ImGui::MenuItem( "Options..." );

			ImGui::EndMenu();
		}
		if ( ImGui::BeginMenu( "Help" ) )
		{
			ImGui::MenuItem( "About...", nullptr, &mapEdit.ui_about );

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

static void ShowMenuBar( bool *pOpen )
{
	if ( ImGui::BeginMenuBar() )
	{
		if ( ImGui::BeginMenu( "File" ) )
		{
			ImGui::MenuItem( "New", "Ctrl+N" );
			ImGui::MenuItem( "Open...", "Ctrl+O" );
			ImGui::MenuItem( "Save", "Ctrl+S" );
			ImGui::MenuItem( "Save As...", nullptr );
			ImGui::MenuItem( "Save All", "Ctrl+Shift+S" );
			ImGui::MenuItem( "Close", "Ctrl+W" );
			ImGui::Separator();
			if ( ImGui::MenuItem( "Exit", "Ctrl+Q" ) )
			{
				*pOpen = false;
			}

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

static void ShowOtherWindows()
{
	if ( mapEdit.ui_about )
	{
		ImGui::SetNextWindowSize( ImVec2( 325.0f, 146.0f ) );
		if ( ImGui::Begin( "Map Editor", &mapEdit.ui_about ) )
		{
			ImGui::TextUnformatted( "Hello" );
		}
		ImGui::End();
	}
}

void ShowMapEditor( bool *pOpen )
{
	// main window

	assert( *pOpen );

	const ImGuiWindowFlags mainWindowFlags =
		ImGuiWindowFlags_MenuBar;

	ImGui::SetNextWindowSize( ImVec2( 1200.0f, 800.0f ) );
	if ( !ImGui::Begin( "Map Editor", pOpen, mainWindowFlags ) )
	{
		ImGui::End();
		return;
	}

	// menu bar

	ShowMenuBar_NoDocument( pOpen );

	// main window internals



	// other windows

	ShowOtherWindows();

	ImGui::End();
}

}
