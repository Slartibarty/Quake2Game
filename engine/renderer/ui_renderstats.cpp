/*
===================================================================================================

	A little stats widget with FPS, frametime, etc

===================================================================================================
*/

#include "gl_local.h"

#include "imgui.h"

namespace UI::StatsPanel
{

void ShowRenderStats()
{
	char workBuf[128];
	int length;

	length = Q_sprintf_s( workBuf, "%-20s: %d", "epoly", tr.pc.aliasPolys);
	ImGui::TextUnformatted( workBuf, workBuf + length );
	length = Q_sprintf_s( workBuf, "%-20s: %d", "wpoly", tr.pc.worldPolys );
	ImGui::TextUnformatted( workBuf, workBuf + length );
	length = Q_sprintf_s( workBuf, "%-20s: %d", "world draw calls", tr.pc.worldDrawCalls );
	ImGui::TextUnformatted( workBuf, workBuf + length );
}

}
