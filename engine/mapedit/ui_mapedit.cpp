/*
===================================================================================================

	ImGui map editor

===================================================================================================
*/

#include "mapedit_local.h"

#include "imgui.h"

namespace UI::MapEdit
{

static struct mapEdit_t
{
	int currentTexture;
	bool ui_about;
} mapEdit;

const char *s_dummyTextureList[]
{
	"brick/brickwall001a.mat",
	"brick/brickwall002a.mat",
	"brick/brickwall003a.mat",
	"brick/brickwall004a.mat",
	"brick/brickwall005a.mat"
};

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

// This toolbar is always available
static void ShowMainToolbar()
{
	if ( ImGui::BeginChild( "##MFToolbar" ) )
	{
		ImGui::Button( "New" ); ImGui::SameLine();
		ImGui::Button( "Open" ); ImGui::SameLine();
		ImGui::Button( "Save" ); ImGui::SameLine();
		ImGui::Separator(); ImGui::SameLine();
		ImGui::Button( "Cut" ); ImGui::SameLine();
		ImGui::Button( "Copy" ); ImGui::SameLine();
		ImGui::Button( "Paste" );
	}
	ImGui::EndChild();
}

static void ShowToolsList()
{
	if ( ImGui::Begin( "Tools List" ) )
	{
		ImGui::Button( "Select" );
		ImGui::Separator();
		ImGui::Button( "Entity" );
		ImGui::Button( "Brush" );
		ImGui::Separator();
		ImGui::Button( "Texture Applicator" );
		ImGui::Button( "Apply Texture" );
		ImGui::Button( "Decal" );
		ImGui::Separator();
		ImGui::Button( "Clipping" );
		ImGui::Button( "Vertex" );
	}
	ImGui::End();
}

static void ShowSelectionList()
{
	if ( ImGui::Begin( "Selection Mode" ) )
	{
		ImGui::TextUnformatted( "Select:" );
		ImGui::Button( "Groups" );
		ImGui::Button( "Objects" );
		ImGui::Button( "Solids" );
	}
	ImGui::End();
}

static void ShowTextureBar()
{
	if ( ImGui::Begin( "Textures" ) )
	{
		ImGui::TextUnformatted( "Current texture:" );
		if ( ImGui::BeginCombo( "##CurTexture", s_dummyTextureList[mapEdit.currentTexture] ) )
		{
			for ( int i = 0; i < countof( s_dummyTextureList ); ++i )
			{
				if ( ImGui::Selectable( s_dummyTextureList[i] ) )
				{
					mapEdit.currentTexture = i;
				}
			}
			ImGui::EndCombo();
		}
	}
	ImGui::End();
}

static void ShowToolWindows( ImGuiID dockID )
{
	ImGui::SetNextWindowDockID( dockID, ImGuiCond_FirstUseEver );
	ShowToolsList();
	ImGui::SetNextWindowDockID( dockID, ImGuiCond_FirstUseEver );
	ShowSelectionList();
	ImGui::SetNextWindowDockID( dockID, ImGuiCond_FirstUseEver );
	ShowTextureBar();
}

static void ShowOtherWindows()
{
	if ( mapEdit.ui_about )
	{
		if ( ImGui::Begin( "About", &mapEdit.ui_about ) )
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

	ImGui::SetNextWindowSize( ImVec2( 1200.0f, 800.0f ), ImGuiCond_FirstUseEver );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
	if ( !ImGui::Begin( "Map Editor", pOpen, mainWindowFlags ) )
	{
		ImGui::PopStyleVar();
		ImGui::End();
		return;
	}
	ImGui::PopStyleVar();

	// menu bar

	ShowMenuBar_NoDocument( pOpen );

	// dockspace

	ImGuiID dockID = ImGui::GetID( "ME_MainDock" );
	ImGui::DockSpace( dockID );

	ShowToolWindows( dockID );

	// main window internals



	// other windows

	ShowOtherWindows();

	ImGui::End();
}

}
