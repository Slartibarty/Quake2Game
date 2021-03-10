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
	buildoptions { "/utf-8", "/permissive", "/Zc:__cplusplus" }
	defines { "WIN32", "_WINDOWS", "_CRT_SECURE_NO_WARNINGS" }
filter {}
	
-- Config for Windows, release, clean this up!
filter { "system:windows", filter_rel_or_rtl }
	buildoptions { "/Gw" }
filter {}

-- Config for all projects in debug, _DEBUG is defined for library-compatibility
filter( filter_dbg )
	defines { "_DEBUG", "Q_DEBUG" }
	symbols "FastLink"
filter {}

-- Config for all projects in release or retail, NDEBUG is defined for cstd compatibility (assert)
filter( filter_rel_or_rtl )
	defines { "NDEBUG", "Q_RELEASE" }
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

local zlib_public = {
	"external/zlib/zconf.h",
	"external/zlib/zlib.h"
}

local zlib_sources = {
	"external/zlib/adler32.c",
	"external/zlib/compress.c",
	"external/zlib/crc32.c",
	"external/zlib/crc32.h",
	"external/zlib/deflate.c",
	"external/zlib/deflate.h",
	"external/zlib/gzclose.c",
	"external/zlib/gzguts.h",
	"external/zlib/gzlib.c",
	"external/zlib/gzread.c",
	"external/zlib/gzwrite.c",
	"external/zlib/infback.c",
	"external/zlib/inffast.c",
	"external/zlib/inffast.h",
	"external/zlib/inffixed.h",
	"external/zlib/inflate.c",
	"external/zlib/inflate.h",
	"external/zlib/inftrees.c",
	"external/zlib/inftrees.h",
	"external/zlib/trees.c",
	"external/zlib/trees.h",
	"external/zlib/uncompr.c",
	"external/zlib/zutil.c",
	"external/zlib/zutil.h"
}

local libpng_public = {
	"external/libpng/png.h",
	"external/libpng/pngconf.h"
}

local libpng_sources = {
	"external/libpng/png.c",
	"external/libpng/pngpriv.h",
	"external/libpng/pngstruct.h",
	"external/libpng/pnginfo.h",
	"external/libpng/pngdebug.h",
	"external/libpng/pngerror.c",
	"external/libpng/pngget.c",
	"external/libpng/pngmem.c",
	"external/libpng/pngpread.c",
	"external/libpng/pngread.c",
	"external/libpng/pngrio.c",
	"external/libpng/pngrtran.c",
	"external/libpng/pngrutil.c",
	"external/libpng/pngset.c",
	"external/libpng/pngtrans.c",
	"external/libpng/pngwio.c",
	"external/libpng/pngwrite.c",
	"external/libpng/pngwtran.c",
	"external/libpng/pngwutil.c"
}

local glew_public = {
	"external/glew/include/GL/glew.h",
	"external/glew/include/GL/wglew.h"
}

local glew_sources = {
	"external/glew/src/glew.c"
}

local xatlas_public = {
	"external/xatlas/xatlas.h",
}

local xatlas_sources = {
	"external/xatlas/xatlas.cpp",
}

local uvatlas_public = {
	"external/UVAtlas/UVAtlas/inc/UVAtlas.h",
}

local uvatlas_sources = {
	"external/UVAtlas/UVAtlas/geodesics/*.cpp",
	"external/UVAtlas/UVAtlas/geodesics/*.h",
	"external/UVAtlas/UVAtlas/isochart/*.cpp",
	"external/UVAtlas/UVAtlas/isochart/*.h"
}

-------------------------------------------------------------------------------

group "game"

project "engine"
	kind "WindowedApp"
	targetname "q2game"
	language "C++"
	targetdir "../game"
	includedirs { "external/UVAtlas/UVAtlas/inc", "external/xatlas", "external/glew/include", "external/zlib", "external/libpng", "external/libpng_config" }
	defines { "GLEW_STATIC", "GLEW_NO_GLU" }
	filter "system:windows"
		linkoptions { "/ENTRY:mainCRTStartup" }
		links { "ws2_32", "dsound", "dxguid", "opengl32", "noenv.obj", "zlib", "libpng", "uvatlas" }
	filter {}
	filter "system:linux"
		links { "sdl2" }
	filter {}
	
	disablewarnings { "4244", "4267" }

	files {
		"common/*",
		"engine/client/*",
		"engine/server/*",
		"engine/res/*",
		"engine/shared/*",
		
		"engine/renderer/*",
		
		"game_shared/game_public.h",
		
		zlib_public,
		
		libpng_public,
		
		xatlas_public,
		xatlas_sources,
		
		glew_public,
		glew_sources,
	}
	
	filter "system:windows"
		removefiles {
			"**/*_linux.*"
		}
	filter {}
	filter "system:linux"
		removefiles {
			"**/*_win.*"
		}
	filter {}
	
	removefiles {
		"engine/client/cd_win.*",
		"engine/res/rw_*",
		"**/cd_vorbis.cpp",
		"**.def",
		"**/*sv_null.*",
		"**_pch.cpp"
	}
	
project "cgame_q2"
	kind "SharedLib"
	filter( filter_32bit )
		targetname "cgamex86"
	filter {}
	filter( filter_64bit )
		targetname "cgamex64"
	filter {}
	language "C++"
	targetdir "../game/baseq2"
		
	pchsource( "cgame_q2/cg_pch.cpp" )
	pchheader( "cg_local.h" )
	filter( "files:not cgame_q2/**" )
		flags( { "NoPCH" } )
	filter( {} )

	files {
		"common/*",
		"cgame_q2/*",
		"cgame_shared/*",
		
		"game_shared/m_flash.cpp"
	}
	
	filter "system:windows"
		removefiles {
			"**/*_linux.*"
		}
	filter {}
	filter "system:linux"
		removefiles {
			"**/*_win.*"
		}
	filter {}
	
	removefiles {
		"**.manifest",
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
	
	filter "system:windows"
		removefiles {
			"**/*_linux.*"
		}
	filter {}
	filter "system:linux"
		removefiles {
			"**/*_win.*"
		}
	filter {}
	
	removefiles {
		"**.manifest",
	}
	
-- Utils

filter "system:windows"

group "utilities"

project "qbsp4"
	kind "ConsoleApp"
	targetname "qbsp4"
	language "C++"
	floatingpoint "Default"
	targetdir "../game"
	includedirs { "utils/common2", "common" }
	
	files {
		"common/windows_default.manifest",
		
		"common/*.cpp",
		"common/*.h",
		
		"utils/common2/cmdlib.*",
		"utils/common2/mathlib.*",
		"utils/common2/scriplib.*",
		"utils/common2/polylib.*",
		"utils/common2/threads.*",
		"utils/common2/bspfile.*",
			
		"utils/qbsp4/*"
	}
	
	filter "system:windows"
		removefiles {
			"**/*_linux.*"
		}
	filter {}
	filter "system:linux"
		removefiles {
			"**/*_win.*"
		}
	filter {}

	
project "qvis4"
	kind "ConsoleApp"
	targetname "qvis4"
	language "C++"
	floatingpoint "Default"
	targetdir "../game"
	includedirs { "utils/common2", "common" }
	
	files {
		"common/windows_default.manifest",
		
		"common/*.cpp",
		"common/*.h",

		"utils/common2/cmdlib.*",
		"utils/common2/mathlib.*",
		"utils/common2/threads.*",
		"utils/common2/scriplib.*",
		"utils/common2/bspfile.*",
	
		"utils/qvis4/*"
	}
	
	filter "system:windows"
		removefiles {
			"**/*_linux.*"
		}
	filter {}
	filter "system:linux"
		removefiles {
			"**/*_win.*"
		}
	filter {}
	
project "qrad4"
	kind "ConsoleApp"
	targetname "qrad4"
	language "C++"
	floatingpoint "Default"
	targetdir "../game"
	includedirs { "utils/common2", "common", "external/stb" }
	
	files {
		"common/windows_default.manifest",
		
		"common/*.cpp",
		"common/*.h",
	
		"utils/common2/cmdlib.*",
		"utils/common2/mathlib.*",
		"utils/common2/threads.*",
		"utils/common2/polylib.*",
		"utils/common2/scriplib.*",
		"utils/common2/bspfile.*",
	
		"utils/qrad4/*"
	}
	
	filter "system:windows"
		removefiles {
			"**/*_linux.*"
		}
	filter {}
	filter "system:linux"
		removefiles {
			"**/*_win.*"
		}
	filter {}
	
project "qatlas"
	kind "ConsoleApp"
	targetname "qatlas"
	language "C++"
	floatingpoint "Default"
	targetdir "../game"
	includedirs { "utils/common2", "common", "external/xatlas" }
	
	files {
		"common/windows_default.manifest",
		"common/q_shared.*",
		"common/q_formats.h",
	
		"utils/common2/cmdlib.*",
		
		"utils/qatlas/*",
		
		xatlas_public,
		xatlas_sources
	}
	
--[[
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

filter {}
--]]

filter {}

-- External

group "external"

project "zlib"
	kind "StaticLib"
	targetname "zlib"
	language "C"
	defines { "_CRT_NONSTDC_NO_WARNINGS" }
	
	disablewarnings { "4267" }
	
	files {
		zlib_public,
		zlib_sources
	}
	
project "libpng"
	kind "StaticLib"
	targetname "libpng"
	language "C"
	includedirs { "external/libpng_config", "external/zlib" }
	
	files {
		libpng_public,
		libpng_sources,
		
		"external/libpng_config/pnglibconf.h"
	}


project "uvatlas"
	kind "StaticLib"
	targetname "uvatlas"
	language "C++"
	includedirs { "external/UVAtlas/UVAtlas", "external/UVAtlas/UVAtlas/inc", "external/UVAtlas/UVAtlas/geodesics" }
	
	disablewarnings { "4530" }
	
	files {
		uvatlas_public,
		uvatlas_sources
	}
