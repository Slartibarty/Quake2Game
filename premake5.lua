-------------------------------------------------------------------------------
-- Premake5 build script for Quake 2
-------------------------------------------------------------------------------

-- Convenience locals
local conf_dbg = "debug"
local conf_rel = "release"
local conf_rtl = "retail"

local plat_32bit = "x86"
local plat_64bit = "x64"

local build_dir = "../build"

local filter_dbg = "configurations:" .. conf_dbg
local filter_rel_or_rtl = "configurations:release or retail" -- TODO: This shouldn't be necessary
local filter_rtl = "configurations:" .. conf_rtl

local filter_32bit = "platforms:" .. plat_32bit
local filter_64bit = "platforms:" .. plat_64bit

-- Workspace definition -------------------------------------------------------

workspace "quake2"
	configurations { conf_dbg, conf_rel, conf_rtl }
	platforms { plat_32bit, plat_64bit }
	location( build_dir )
	preferredtoolarchitecture "x86_64"
	startproject "engine"

-- Configuration --------------------------------------------------------------

-- Misc flags for all projects

includedirs { "external/stb" }

flags { "MultiProcessorCompile", "NoBufferSecurityCheck" }
staticruntime "On"
cppdialect "C++20"
warnings "Default"
floatingpoint "Fast"
characterset "ASCII"
exceptionhandling "Off"

-- Config for all 32-bit projects
filter( filter_32bit )
	vectorextensions "SSE2"
	architecture "x86"
filter {}
	
-- Config for all 64-bit projects
filter( filter_64bit )
	architecture "x86_64"
filter {}

-- Config for Windows
filter "system:windows"
	buildoptions { "/permissive", "/Zc:__cplusplus" }
	defines { "WIN32", "_WINDOWS", "_CRT_SECURE_NO_WARNINGS" }
	links { "noenv.obj" }
filter {}
	
-- Config for Windows, release, clean this up!
filter { "system:windows", filter_rel_or_rtl }
	buildoptions { "/Gw", "/Zc:inline" }
filter {}

-- Config for all projects in debug, _DEBUG is defined for library-compatibility
filter( filter_dbg )
	defines { "_DEBUG" }
	symbols "FastLink"
filter {}

-- Config for all projects in release or retail, NDEBUG is defined for cstd compatibility (assert)
filter( filter_rel_or_rtl )
	defines { "NDEBUG" }
	symbols "Full"
	optimize "Speed"
filter {}

-- Config for all projects in retail
filter( filter_rtl )
	defines { "Q_RETAIL" }
	symbols "Off"
	flags { "LinkTimeOptimization" }
filter {}

-- Config for shared library projects
filter "kind:SharedLib"
	flags { "NoManifest" } -- We don't want manifests for DLLs
filter {}

-- Project definitions --------------------------------------------------------

group "game"
	
project "engine"
	kind "WindowedApp"
	targetname "q2game"
	language "C++"
	targetdir "../game"
	linkoptions { "/ENTRY:mainCRTStartup" }
	defines { "_WINSOCK_DEPRECATED_NO_WARNINGS" }
	links { "ws2_32", "winmm", "dsound", "dxguid" }
	
	disablewarnings { "4244", "4267" }

	files {
		"common/*",
		"engine/client/*",
		"engine/server/*",
		"engine/res/*",
		"engine/shared/*",
		
		"engine/ref_shared/anorms.inl",
		"engine/ref_shared/ref_public.h",
		
		"game_shared/game_public.h",
		"game_shared/m_flash.cpp"
	}
	
	removefiles {
		"engine/client/cd_win.*",
		"engine/res/rw_*",
		"engine/shared/pmove_hl1.cpp",
		"**/sv_null.*",
		"**_pch.cpp"
	}
	
project "ref_gl"
	kind "SharedLib"
	targetname "ref_gl"
	language "C++"
	targetdir "../game"
	includedirs { "external/glew/include" }
	defines { "GLEW_STATIC", "GLEW_NO_GLU" }
	links { "opengl32" }
	
	disablewarnings { "4244" }

	files {
		"common/*",
		"engine/shared/imageloaders.*",
		"engine/shared/misc_win.cpp",
		"engine/res/resource.h",
		
		"engine/ref_gl/*",
		"engine/ref_shared/*",
		
		"external/glew/src/glew.c"
	}
	
	removefiles {
		"**.manifest",
	
		"**_null.*",
		"**_pch.cpp"
	}
	
project "game_q2"
	kind "SharedLib"
	filter( filter_32bit )
		targetname "gamex86"
	filter {}
	filter( filter_64bit )
		targetname "gamex64"
	filter {}
	language "C++"
	targetdir "../game/baseq2"
	
	disablewarnings { "4244", "4311", "4302" }
	
	pchsource( "game_q2/g_pch.cpp" )
	pchheader( "g_local.h" )
	filter( "files:not game_q2/**" )
		flags( { "NoPCH" } )
	filter( {} )

	files {
		"common/*",
		"game_q2/*",
		"game_shared/*"
	}
	
	removefiles {
		"**.manifest",
		"game_q2/p_view_hl1.cpp",
		
		"**_null.*",
		--"**_pch.cpp"
	}
	
-- Utils

group "utilities"

project "qbsp3"
	kind "ConsoleApp"
	targetname "qbsp3"
	language "C"
	floatingpoint "Default"
	targetdir "../game"
	includedirs "utils/common"
	
	files {
		"common/windows_default.manifest",
		
		"utils/common/cmdlib.*",
		"utils/common/mathlib.*",
		"utils/common/scriplib.*",
		"utils/common/polylib.*",
		"utils/common/threads.*",
		"utils/common/bspfile.*",
		
		"utils/common/qfiles.h",
	
		"utils/qbsp3/*"
	}
	
project "qvis3"
	kind "ConsoleApp"
	targetname "qvis3"
	language "C"
	floatingpoint "Default"
	targetdir "../game"
	includedirs "utils/common"
	
	files {
		"common/windows_default.manifest",

		"utils/common/cmdlib.*",
		"utils/common/mathlib.*",
		"utils/common/threads.*",
		"utils/common/scriplib.*",
		"utils/common/bspfile.*",
	
		"utils/qvis3/*"
	}
	
project "qrad3"
	kind "ConsoleApp"
	targetname "qrad3"
	language "C"
	floatingpoint "Default"
	targetdir "../game"
	includedirs { "utils/common", "external/stb" }
	
	files {
		"common/windows_default.manifest",
	
		"utils/common/cmdlib.*",
		"utils/common/mathlib.*",
		"utils/common/threads.*",
		"utils/common/polylib.*",
		"utils/common/scriplib.*",
		"utils/common/bspfile.*",
		"utils/common/lbmlib.*",
	
		"utils/qrad3/*"
	}
	
project "light"
	kind "ConsoleApp"
	targetname "light"
	language "C"
	floatingpoint "Default"
	targetdir "../game"
	defines { "QE4" } -- We want the alternate VectorNormalize
	includedirs { "utils/common" }
	
	files {
		"common/windows_default.manifest",
	
		"utils/common/cmdlib.*",
		"utils/common/mathlib.*",
		"utils/common/scriplib.*",
		"utils/common/threads.*",
		"utils/common/bspfile.*",
	
		"utils/qrad3/trace.c",
		"utils/light/*"
	}
	
project "qdata"
	kind "ConsoleApp"
	targetname "qdata"
	language "C"
	floatingpoint "Default"
	targetdir "../game"
	includedirs { "utils/common", "external/stb" }
	
	files {
		"common/windows_default.manifest",
	
		"utils/common/cmdlib.*",
		"utils/common/scriplib.*",
		"utils/common/mathlib.*",
		"utils/common/trilib.*",
		"utils/common/lbmlib.*",
		"utils/common/threads.*",
		"utils/common/l3dslib.*",
		"utils/common/bspfile.*",
		"utils/common/md4.*",
	
		"utils/qdata/*"
	}

project "qe4"
	kind "WindowedApp"
	targetname "qe4"
	language "C"
	floatingpoint "Default"
	targetdir "../game"
	defines { "WIN_ERROR", "QE4" }
	includedirs { "utils/common", "external/stb" }
	links { "opengl32", "glu32" }
	
	files {
		"common/*.manifest",
		
		"utils/common/cmdlib.*",
		"utils/common/mathlib.*",
		"utils/common/bspfile.h",
		"utils/common/lbmlib.*",
		"utils/common/qfiles.*",
		
		"utils/qe4/*"
	}
