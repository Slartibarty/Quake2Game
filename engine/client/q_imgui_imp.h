//=================================================================================================
// Think Tank 2
// Copyright (c) Slartibarty. All Rights Reserved.
//=================================================================================================

#pragma once

namespace qImGui
{

	bool		OSImp_Init( void *window );
	void		OSImp_Shutdown();
	void		OSImp_SetWantMonitorUpdate();
	void		OSImp_NewFrame();

#ifdef _WIN32
	bool		OSImp_UpdateMouseCursor();
#endif

}
