/*
===================================================================================================

	A little stats widget with FPS, frametime, etc

===================================================================================================
*/

#include "cl_local.h"

#include "imgui.h"

namespace UI::StatsPanel
{

#define FPS_FRAMES 6

void ShowStats()
{
	const ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove;

	ImGui::SetNextWindowPos( ImVec2( 520.0f, 64.0f ), ImGuiCond_Always );
	ImGui::SetNextWindowBgAlpha( 0.25f );

	ImGui::Begin( "Stats Area", nullptr, windowFlags );

	char workBuf[128];
	int length;

	// calc FPS
	{
		static double previousTimes[FPS_FRAMES];
		static uint index;
		static double previous;

		// don't use serverTime, because that will be drifting to
		// correct for internet lag changes, timescales, timedemos, etc
		double t = Time_FloatMilliseconds();
		double frameTime = t - previous;
		previous = t;

		previousTimes[index++ % FPS_FRAMES] = frameTime;
		if ( index > FPS_FRAMES )
		{
			// average multiple frames together to smooth changes out a bit
			double total = 0.0;
			for ( uint i = 0; i < FPS_FRAMES; ++i ) {
				total += previousTimes[i];
			}
			if ( total == 0.0 ) {
				total = 1.0;
			}
			double fps = 1000000.0 * FPS_FRAMES / total;
			fps = ( fps + 500.0 ) / 1000.0;

			length = Q_sprintf_s( workBuf, "Client FPS : %f", fps );
			ImGui::TextUnformatted( workBuf, workBuf + length );
			ImGui::Separator();
		}
	}

	length = Q_sprintf_s( workBuf, "cls.framecount : %d", cls.framecount );
	ImGui::TextUnformatted( workBuf, workBuf + length );
	length = Q_sprintf_s( workBuf, "cls.realtime   : %d", cls.realtime );
	ImGui::TextUnformatted( workBuf, workBuf + length );
	length = Q_sprintf_s( workBuf, "cls.frametime  : %f", cls.frametime );
	ImGui::TextUnformatted( workBuf, workBuf + length );

	ImGui::End();
}

}
