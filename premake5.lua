-------------------------------------------------------------------------------
-- Premake5 build script for Quake 2
-------------------------------------------------------------------------------

-- Convenience locals
local conf_dbg = "debug"
local conf_rel = "release"
local conf_rtl = "retail"

local plat_win64 = "x64"

local build_dir = "../build"

-- Workspace definition -------------------------------------------------------

workspace( "quake2" )
	configurations( { conf_dbg, conf_rel, conf_rtl } )
	platforms( { plat_win64 } )
	location( build_dir )
	preferredtoolarchitecture( "x86_64" )

-- Configuration --------------------------------------------------------------

-- Misc flags for all projects
flags( { "MultiProcessorCompile", "NoBufferSecurityCheck" } )
staticruntime( "On" )
cppdialect( "C++20" )
--compileas( "C++" )
warnings( "Default" )
floatingpoint( "Fast" )
characterset( "ASCII" )
exceptionhandling( "Off" )

-- Config for all 64-bit projects
filter( "platforms:" .. plat_win64 )
	vectorextensions( "AVX2" )
	architecture( "x86_64" )

-- Config for Windows
filter( "system:windows" )
	buildoptions( { "/Zc:preprocessor" } )
	defines( { "WIN32", "_WINDOWS", "_CRT_SECURE_NO_WARNINGS" } )
	
-- Config for Windows, release, clean this up!
filter( { "system:windows", "configurations:" .. conf_rel } )
	buildoptions( { "/Gw" } )

-- Config for all projects in debug
filter( "configurations:" .. conf_dbg )
	defines( { "_DEBUG" } )
	symbols( "FastLink" )

-- Config for all projects in release AND retail
filter( "configurations:" .. conf_rel .. " or configurations:" .. conf_rtl )
	defines( { "NDEBUG" } )
	symbols( "Full" )
	optimize( "Speed" )

-- Config for all projects in retail
filter( "configurations:" .. conf_rtl )
	symbols( "Off" )
	flags( { "LinkTimeOptimization" } )

-- Config for shared library projects
filter( "kind:SharedLib" )
	flags( { "NoManifest" } ) -- We don't want manifests for DLLs

-- Project definitions --------------------------------------------------------
	
project( "engine" )
	kind( "WindowedApp" )
	targetname( "q2game" )
	language( "C++" )
	targetdir( "../game" )
	linkoptions( { "/ENTRY:mainCRTStartup" } )
	includedirs( { "external/stb" } )
	defines( { "_WINSOCK_DEPRECATED_NO_WARNINGS" } )
	links( { "ws2_32", "winmm", "dsound", "dxguid" } )

	files( {
		"common/*",
		"engine/client/*",
		"engine/server/*",
		"engine/res/*",
		"engine/shared/*",
		
		"engine/ref_shared/anorms.inl",
		"engine/ref_shared/ref_public.h",
		
		"game_shared/game_public.h",
		"game_shared/m_flash.cpp"
	} )
	
	removefiles( {
		"engine/client/cd_win.*",
		"engine/res/rw_*",
		"engine/shared/pmove_hl1.cpp",
		"**/sv_null.*",
		"**_pch.cpp"
	} )
	
project( "ref_gl" )
	kind( "SharedLib" )
	targetname( "ref_gl" )
	language( "C++" )
	targetdir( "../game" )
	includedirs( { "external/stb", "external/glew/include" } )
	defines( { "GLEW_STATIC", "GLEW_NO_GLU" } )
	links( { "opengl32" } )

	files( {
		"common/*",
		"engine/shared/imageloaders.*",
		"engine/shared/misc_win.cpp",
		"engine/res/resource.h",
		
		"engine/ref_gl/*",
		"engine/ref_shared/*",
		
		"external/glew/src/glew.c",
	} )
	
	removefiles( {
		"**.manifest",
	
		"**_null.*",
		"**_pch.cpp"
	} )
	
project( "game" )
	kind( "SharedLib" )
	targetname( "game" .. plat_win64 )
	language( "C++" )
	targetdir( "../game/baseq2" )

	files( {
		"common/*",
		"game_q2/*",
		"game_shared/*"
	} )
	
	removefiles( {
		"**.manifest",
		"game_q2/p_view_hl1.cpp",
		
		"**_null.*",
		"**_pch.cpp"
	} )
	
-- Utils

group( "Utilities" )

project( "qbsp3" )
	kind( "ConsoleApp" )
	targetname( "qbsp3" )
	language( "C++" )
	targetdir( "../game" )
	includedirs( "utils/common" )
	
	files( {
		"utils/common/cmdlib.*",
		"utils/common/mathlib.*",
		"utils/common/scriplib.*",
		"utils/common/polylib.*",
		"utils/common/threads.*",
		"utils/common/bspfile.*",
	
		"utils/qbsp3/*"
	} )

