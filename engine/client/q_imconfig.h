//=================================================================================================
// Think Tank 2
// Copyright (c) Slartibarty. All Rights Reserved.
//=================================================================================================

#pragma once

//-------------------------------------------------------------------------------------------------
// This file is dedicated to Jerma and his dumptruck ass, amen
//-------------------------------------------------------------------------------------------------

#include "../../core/core.h"

#include "../../common/filesystem_interface.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#define IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS
#define IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS

#define IMGUI_STB_TRUETYPE_FILENAME		"../stb/stb_truetype.h"
#define IMGUI_STB_RECT_PACK_FILENAME	"../stb/stb_rect_pack.h"

#ifdef Q_RETAIL
//#define IMGUI_DISABLE_DEMO_WINDOWS
#endif

#ifdef Q_DEBUG
#define IMGUI_DEBUG_TOOL_ITEM_PICKER_EX
#define IMGUI_DEBUG_PARANOID
#endif

//#define IMGUI_ENABLE_FREETYPE

#ifdef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS

// Provide our own file methods

using ImFileHandle = fsHandle_t;

extern ImFileHandle		ImFileOpen( const char *filename, const char *mode );
extern bool				ImFileClose( ImFileHandle file );
extern uint64			ImFileGetSize( ImFileHandle file );
extern uint64			ImFileRead( void *data, uint64 size, uint64 count, ImFileHandle file );
extern uint64			ImFileWrite( const void *data, uint64 size, uint64 count, ImFileHandle file );

#endif
