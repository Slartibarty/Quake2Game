/*
===================================================================================================

	A modal dialog that lets you browse materials

	MAYBE: Make this non modal?

===================================================================================================
*/

#include "mapedit_local.h"

#include "imgui.h"

namespace UI::MapEdit::MaterialBrowser
{
	static const char *s_dummyTextureList[]
	{
		"brick/brickwall001a.mat",
		"brick/brickwall002a.mat",
		"brick/brickwall003a.mat",
		"brick/brickwall004a.mat",
		"brick/brickwall005a.mat"
	};

	enum thumbnailSize_t : int
	{
		sz64,
		sz128,
		sz192,
		sz256,
		sz384
	};

	static struct materialBrowser_t
	{
		bool cvarsInitted = false;

		int thumbSize = 128;
	} mb;

	cvar_t *mb_thumbMin;
	cvar_t *mb_thumbMax;

	// TEMP until we get autocvars in
	static void SetupCvars()
	{
		if ( !mb.cvarsInitted )
		{
			mb_thumbMin = Cvar_Get( "mb_thumbMin", "64", 0, "Min material browser thumbnail dimension." );
			mb_thumbMax = Cvar_Get( "mb_thumbMax", "384", 0, "Max material browser thumbnail dimension." );

			mb.cvarsInitted = true;
		}
	}

	// Returns the number of elements that can fit within a specified window width
	static int ElementsForWidth( int thumbDim, int windowWidth )
	{
		// the spacing between elements
		const int spacingX = static_cast<int>( ImGui::GetStyle().ItemSpacing.x );

		// add spacing to each thumbnail
		thumbDim += spacingX;
		// account for window spacing
		windowWidth -= spacingX;

		// compute how many thumbnails we can fit, minimum of 1
		return Max( 1, windowWidth / thumbDim );
	}

	void ShowModal( bool *pOpen )
	{
		SetupCvars();

		ImGui::SetNextWindowSize( ImVec2( 1365.0f, 725.0f ), ImGuiCond_FirstUseEver );
		if ( ImGui::Begin( "Material Browser", pOpen ) )
		{
			ImGui::SliderInt( "Thumbnail Size", &mb.thumbSize, mb_thumbMin->GetInt(), mb_thumbMax->GetInt(), "%d", ImGuiSliderFlags_AlwaysClamp );

			ImGui::BeginChild( "Preview Pane", ImVec2( 0.0f, -64.0f ), true );

			const int imageDimensions = mb.thumbSize;

			// compute how many images we can fit into the child area
			const int fitCount = ElementsForWidth( imageDimensions, static_cast<int>( ImGui::GetWindowWidth() ) );



			for ( int i = 0; i < 16; ++i )
			{
				if ( i % fitCount )
				{
					ImGui::SameLine();
				}
				ImGui::Image( (ImTextureID)R_GetDefaultTexture(), ImVec2( imageDimensions, imageDimensions ) );
			}

			ImGui::EndChild();
		}
		ImGui::End();
	}

}
