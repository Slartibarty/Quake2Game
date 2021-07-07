/*
===================================================================================================

	Simple model viewer

===================================================================================================
*/

#include "cl_local.h"

#include "imgui.h"

extern float CalcFov( float fov_x, float w, float h );

namespace UI::ModelViewer
{

#define FB_SIZE 512

static const filterSpec_t s_modelTypes[]
{
	{ PLATTEXT( "SMF (*.smf)" ), PLATTEXT( "*.smf" ) },
	{ PLATTEXT( "MD2 (*.md2)" ), PLATTEXT( "*.md2" ) },
	{ PLATTEXT( "All Model Files (*.smf;*.md2)" ), PLATTEXT( "*.smf;*.md2" ) }
};

struct modelViewer_t
{
	std::string mdlName;
	int frameBuffer;
};

static modelViewer_t mdlViewer;

void ShowModelViewer( bool *pOpen )
{
	static float yaw;

	if ( ImGui::Begin( "Model Viewer", pOpen, ImGuiWindowFlags_MenuBar ) )
	{
		if ( ImGui::BeginMenuBar() )
		{
			if ( ImGui::BeginMenu( "File" ) )
			{
				if ( ImGui::MenuItem( "Open..." ) )
				{
					std::string mdlName;
					Sys_FileOpenDialog( mdlName, PLATTEXT( "Model Picker" ), s_modelTypes, countof( s_modelTypes ), 1 );
					if ( !mdlName.empty() )
					{
						const char *relativePath = FileSystem::AbsolutePathToRelativePath( mdlName.c_str() );
						if ( relativePath )
						{
							mdlViewer.mdlName.assign( relativePath );
						}
						else
						{
							mdlViewer.mdlName.clear();
							Com_Print( S_COLOR_RED "[MDLViewer] Selected model is not located in the game folder!\n" );
						}
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		if ( !mdlViewer.mdlName.empty() )
		{
			entity_t entity{};
			entity.model = R_RegisterModel( mdlViewer.mdlName.c_str() );
			entity.origin[0] = 80.0f;

			entity.angles[1] = yaw;
			if ( ++yaw > 360.0f ) {
				yaw -= 360.0f;
			}

			refdef_t refdef{};
			refdef.width = FB_SIZE;
			refdef.height = FB_SIZE;
			refdef.fov_x = 40;
			refdef.fov_y = CalcFov( 40, FB_SIZE, FB_SIZE );

			refdef.vieworg[2] = 64.0f;

			refdef.time = MS2SEC( static_cast<float>( cl.time ) );
			refdef.frametime = cls.frametime;
			refdef.rdflags = RDF_NOWORLDMODEL;

			refdef.entities = &entity;
			refdef.num_entities = 1;

			if ( !mdlViewer.frameBuffer )
			{
				// create the framebuffer, destroyed at program quit (ouch?)
				mdlViewer.frameBuffer = R_CreateFBO( FB_SIZE, FB_SIZE );
			}

			R_BindFBO( mdlViewer.frameBuffer );
			R_RenderFrame( &refdef );
			R_BindDefaultFBO();

			ImGui::Image( (ImTextureID)(intptr_t)R_TexNumFBO( mdlViewer.frameBuffer ), ImVec2( FB_SIZE, FB_SIZE ), ImVec2( 0.0f, 0.0f ), ImVec2( 1.0f, -1.0f ) );
		}
	}
	ImGui::End();
}

}
