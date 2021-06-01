//=================================================================================================
// Think Tank 2
// Copyright (c) Slartibarty. All Rights Reserved.
//=================================================================================================

#pragma once

//-------------------------------------------------------------------------------------------------
// This file is dedicated to Jerma and his dumptruck ass, amen
//-------------------------------------------------------------------------------------------------

#include "../../core/sys_types.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#define IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS
//#define IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS

#define IMGUI_STB_TRUETYPE_FILENAME		"stb_truetype.h"
#define IMGUI_STB_RECT_PACK_FILENAME	"stb_rect_pack.h"

#ifdef Q_RETAIL
#define IMGUI_DISABLE_DEMO_WINDOWS
#endif

#ifdef Q_DEBUG
#define IMGUI_DEBUG_TOOL_ITEM_PICKER_EX
#define IMGUI_DEBUG_PARANOID
#endif

#ifdef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS

// Provide our own file methods

typedef fsHandle ImFileHandle;
extern ImFileHandle		ImFileOpen( const char *filename, const char *mode );
extern bool				ImFileClose( ImFileHandle file );
extern uint64			ImFileGetSize( ImFileHandle file );
extern uint64			ImFileRead( void *data, uint64 size, uint64 count, ImFileHandle file );
extern uint64			ImFileWrite( const void *data, uint64 size, uint64 count, ImFileHandle file );

#endif
