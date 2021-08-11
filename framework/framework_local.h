/*
===================================================================================================

	The Framework

	Q_ENGINE is defined for the engine project only

===================================================================================================
*/

#pragma once

#ifdef Q_ENGINE

// If we're the engine, just use the engine header
#include "../engine/shared/engine.h"

#else

#include "../../core/core.h"
#include "../../common/cvardefs.h"

// Engine interop defines

#define Com_ServerState() 0

#define BASE_MODDIR "base"
#define ENGINE_VERSION "JaffaUtilities"

#endif

#include "framework_public.h"
