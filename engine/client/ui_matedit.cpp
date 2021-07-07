/*
===================================================================================================

	ImGui material editor

===================================================================================================
*/

#include "cl_local.h"

#include <vector>
#include <string>

#include "imgui.h"

namespace UI::MatEdit
{

static const char *s_materialTemplate
{
	"{\n"
	"\t$basetexture %s\n"
	"}\n"
};

static const filterSpec_t s_textureTypes[]
{
	{ PLATTEXT( "DDS (*.dds)" ), PLATTEXT( "*.dds" ) },
	{ PLATTEXT( "PNG (*.png)" ), PLATTEXT( "*.png" ) },
	{ PLATTEXT( "TGA (*.tga)" ), PLATTEXT( "*.tga" ) },
	{ PLATTEXT( "All Texture Files (*.dds;*.png;*.tga)" ), PLATTEXT( "*.dds;*.png;*.tga" ) },
	{ PLATTEXT( "All Files (*.*)" ), PLATTEXT( "*.*" ) }
};

static const filterSpec_t s_materialTypes[]
{
	{ PLATTEXT( "Material File (*.mat)" ), PLATTEXT( "*.mat" ) }
};

static struct matEdit_t
{
	bool ui_batch;

	std::vector<std::string> batchNames;
} matEdit;

static void CreateMaterialsForTextures()
{
	const uint numFilenames = static_cast<uint>( matEdit.batchNames.size() );

	for ( uint i = 0; i < numFilenames; ++i )
	{
		// trim the filename up to the game directory
		const char *relativePath = FileSystem::AbsolutePathToRelativePath( matEdit.batchNames[i].c_str() );
		if ( !relativePath ) {
			continue;
		}

		fsHandle_t handle = FileSystem::OpenFileWrite( relativePath, FS_GAMEDIR );
		if ( !handle ) {
			continue;
		}

		FileSystem::PrintFileFmt( handle, s_materialTemplate, relativePath );
		FileSystem::CloseFile( handle );
	}
}

static void ShowBatch( bool *pOpen )
{
	ImGui::PushStyleVar( ImGuiStyleVar_WindowMinSize, ImVec2( 330.0f, 250.0f ) );
	if ( !ImGui::Begin( "Material Batch Tool", pOpen ) )
	{
		ImGui::PopStyleVar();
		ImGui::End();
		return;
	}
	ImGui::PopStyleVar();

	if ( ImGui::Button( "Select Files..." ) )
	{
		matEdit.batchNames.clear();
		Sys_FileOpenDialogMultiple( matEdit.batchNames, PLATTEXT( "Texture Picker" ), s_textureTypes, countof( s_textureTypes ), 4 );
	}

	if ( ImGui::Button( "Clear" ) )
	{
		matEdit.batchNames.clear();
	}

	const ImGuiWindowFlags scrollFlags = ImGuiWindowFlags_HorizontalScrollbar;

	if ( ImGui::BeginChild( "ScrollingRegion", ImVec2( 0.0f, -ImGui::GetFrameHeightWithSpacing() ), true, scrollFlags ) )
	{
		for ( uint i = 0; i < matEdit.batchNames.size(); ++i )
		{
			ImGui::TextUnformatted( matEdit.batchNames[i].c_str(), matEdit.batchNames[i].c_str() + matEdit.batchNames[i].length() );
		}
	}
	ImGui::EndChild();

	if ( ImGui::Button( "Create Basic Materials" ) && !matEdit.batchNames.empty() )
	{
		CreateMaterialsForTextures();
		matEdit.batchNames.clear();
	}

	ImGui::End();
}

void ShowMaterialEditor( bool *pOpen )
{
	// main window

	assert( *pOpen );

	const ImGuiWindowFlags mainWindowFlags =
		ImGuiWindowFlags_MenuBar;

	ImGui::PushStyleVar( ImGuiStyleVar_WindowMinSize, ImVec2( 1200.0f, 800.0f ) );
	if ( !ImGui::Begin( "Material Editor", pOpen, mainWindowFlags ) )
	{
		ImGui::PopStyleVar();
		ImGui::End();
		return;
	}
	ImGui::PopStyleVar();

	// menu bar

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
		if ( ImGui::BeginMenu( "Tools" ) )
		{
			ImGui::MenuItem( "Material Batch Tool...", nullptr, &matEdit.ui_batch );

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	// pop-up windows

	if ( matEdit.ui_batch )
	{
		ShowBatch( &matEdit.ui_batch );
	}

	ImGui::End();
}

} // namespace UI::MatEdit
