
#pragma once

// Core
#include "../../core/core.h"

// Utils core
#include "cmdlib.h"

// Framework
#include "../../framework/framework_public.h"

// C++
#include <vector>
#include <string>

// External libs
#include "fbxsdk.h"
#include "meshoptimizer.h"

// This program
#include "fbxutils.h"
#include "utils.h"

// Contains settings relevant to building a model
struct modelBuild_t
{
	char *scriptPath = nullptr;				// The path from the working directory to this script file

	std::vector<std::string> scenes;
	std::vector<std::string> animations;

	bool hasSkeleton = false;
};
