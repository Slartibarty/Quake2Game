/*
===================================================================================================

	A little stats widget with FPS, frametime, etc

===================================================================================================
*/

#include "cl_local.h"

#include "imgui.h"

namespace UI::StatsPanel
{

void ShowStats()
{
	const ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove;

	ImGui::SetNextWindowPos( ImVec2( 520.0f, 64.0f ), ImGuiCond_Always );
	ImGui::SetNextWindowBgAlpha( 0.25f );

	ImGui::Begin( "Stats Area", nullptr, windowFlags );

	// my brain fell out
	// TODO: does this actually work?
	float flFps = 1.0f / ( cls.frametime + FLT_EPSILON );
	int fps = static_cast<int>( flFps );

	char workBuf[128];
	int length;

	length = Q_sprintf_s( workBuf, "Client FPS : %d", fps );
	ImGui::TextUnformatted( workBuf, workBuf + length );
	ImGui::Separator();
	length = Q_sprintf_s( workBuf, "cls.framecount : %d", cls.framecount );
	ImGui::TextUnformatted( workBuf, workBuf + length );
	length = Q_sprintf_s( workBuf, "cls.realtime   : %d", cls.realtime );
	ImGui::TextUnformatted( workBuf, workBuf + length );
	length = Q_sprintf_s( workBuf, "cls.frametime  : %f", cls.frametime );
	ImGui::TextUnformatted( workBuf, workBuf + length );

	ImGui::End();
}

}
