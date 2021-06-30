/*
===================================================================================================

	ImGui material editor

===================================================================================================
*/

#include "cl_local.h"

#include <vector>
#include <string>

#include "imgui.h"

#ifndef _WIN32
#error The material editor is not compatible with Linux yet!
#endif

#include "../../core/sys_includes.h"
#include <CommCtrl.h>
#include <ShlObj_core.h>

#include "winquake.h"

struct filterSpec_t
{
	const platChar_t *pszName;
	const platChar_t *pszSpec;
};

static void Sys_OpenFileDialog(
	std::vector<std::string> &fileNames,
	const filterSpec_t *supportedTypes,
	const uint numTypes,
	const uint defaultIndex )
{
	char filenameBuffer[1024];

	IFileOpenDialog *pDialog;
	HRESULT hr = CoCreateInstance(
		CLSID_FileOpenDialog,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS( &pDialog ) );

	const COMDLG_FILTERSPEC *filterSpec = reinterpret_cast<const COMDLG_FILTERSPEC *>( supportedTypes );

	if ( SUCCEEDED( hr ) )
	{
		FILEOPENDIALOGOPTIONS flags;
		hr = pDialog->GetOptions( &flags ); assert( SUCCEEDED( hr ) );
		hr = pDialog->SetOptions( flags | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT ); assert( SUCCEEDED( hr ) );
		hr = pDialog->SetFileTypes( numTypes, filterSpec ); assert( SUCCEEDED( hr ) );

		hr = pDialog->SetTitle( L"Texture Picker" );

		hr = pDialog->SetFileTypeIndex( defaultIndex ); assert( SUCCEEDED( hr ) );
		//hr = pDialog->SetDefaultExtension( L"dds" ); assert( SUCCEEDED( hr ) );

		hr = pDialog->Show( cl_hwnd );
		if ( SUCCEEDED( hr ) )
		{
			IShellItemArray *pArray;
			hr = pDialog->GetResults( &pArray );
			if ( SUCCEEDED( hr ) )
			{
				DWORD numItems;
				pArray->GetCount( &numItems );
				fileNames.reserve( numItems );
				for ( DWORD i = 0; i < numItems; ++i )
				{
					IShellItem *pItem;
					pArray->GetItemAt( i, &pItem );
					LPWSTR pName;
					pItem->GetDisplayName( SIGDN_FILESYSPATH, &pName );
					WideCharToMultiByte( CP_UTF8, 0, pName, wcslen( pName ) + 1, filenameBuffer, sizeof( filenameBuffer ), nullptr, nullptr );
					CoTaskMemFree( pName );
					Str_FixSlashes( filenameBuffer );
					fileNames.push_back( filenameBuffer );
				}
				pArray->Release();
			}
		}
		pDialog->Release();
	}
}

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

struct matEdit_t
{
	bool ui_batch;

	std::vector<std::string> batchNames;
};

static matEdit_t matEdit;

static void CreateMaterialsForTextures()
{
	const uint numFilenames = static_cast<uint>( matEdit.batchNames.size() );

	char workingDirectory[LONG_MAX_PATH];
	Sys_GetWorkingDirectory( workingDirectory, sizeof( workingDirectory ) );

	for ( uint i = 0; i < numFilenames; ++i )
	{
		// COPY
		std::string filename = matEdit.batchNames[i];

		size_t lastDot = filename.find_last_of( '.' );
		if ( !lastDot )
		{
			continue;
		}

		std::string ext = filename.substr( lastDot, filename.length() );
		filename.erase( filename.begin() + lastDot, filename.end() );
		filename.append( ".mat" );

		// trim the filename up to the game directory

		if ( !filename.starts_with( workingDirectory ) )
		{
			continue;
		}

		FILE *handle = fopen( filename.c_str(), "wb" );
		if ( !handle )
		{
			continue;
		}

		filename.erase( filename.begin() + lastDot, filename.end() );
		filename.append( ext );

		filename.erase( filename.begin(), filename.begin() + strlen( workingDirectory ) + 1 );

		fprintf( handle, s_materialTemplate, filename.c_str() + filename.find( '/' ) + 1 );

		fclose( handle );
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
		Sys_OpenFileDialog( matEdit.batchNames, s_textureTypes, countof( s_textureTypes ), 4 );
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
			ImGui::MenuItem( "Save All...", "Ctrl+Shift+S" );
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
