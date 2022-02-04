-------------------------------------------------------------------------------
-- Premake5 build script for Quake 2
-------------------------------------------------------------------------------

-- Convenience locals

-- Configurations, filters must be updated if these change
local conf_dbg = "debug"
local conf_rel = "release"
local conf_rtl = "retail"

-- Platforms
local plat_64bit = "x64"

-- Directories
local build_dir = "build"
local out_dir = "../game"

-- Filters for each config
local filter_dbg = "configurations:debug"
local filter_rel_or_rtl = "configurations:release or retail"
local filter_rtl = "configurations:retail"

-- Filters for each platform
local filter_64bit = "platforms:" .. plat_64bit

-- Options --------------------------------------------------------------------

newoption {
	trigger = "exclude-utils",
	description = "Exclude utilities"
}
newoption {
	trigger = "profile",
	description = "Enable profiling"
}

newoption {
	trigger = "distro",
	value = "platform",
	description = "Set distribution platform",
	default = "none",
	allowed = {
		{ "none", "No distribution platform" },
		{ "steam", "Build for Steam" }
	}
}

function LinkToProfiler()
	if _OPTIONS["profile"] then
		includedirs { "thirdparty/steamworks/include" }
		defines { "TRACY_ENABLE" }
		files { "thirdparty/tracy/TracyClient.cpp" }
		editandcontinue "Off"
	end
end

function LinkToDistro()
	if _OPTIONS["distro"] == "steam" then
		includedirs { "thirdparty/steamworks/include" }
		defines { "Q_USE_STEAM", "Q_STEAM_APPID=480" }
		filter "system:windows"
			links { "thirdparty/steamworks/lib/win64/steam_api64" }
		filter {}
		filter "system:linux"
			links { "thirdparty/steamworks/lib/linux64/libsteam_api64" }
		filter {}
	end
end

-- Workspace definition -------------------------------------------------------

workspace "jaffaquake"
	configurations { conf_dbg, conf_rel, conf_rtl }
	platforms { plat_64bit }
	location( build_dir )
	preferredtoolarchitecture "x86_64"
	startproject "engine"

-- Configuration --------------------------------------------------------------

-- Misc flags for all projects

includedirs { "thirdparty/stb", "thirdparty/DirectXMath" }

flags { "MultiProcessorCompile", "NoBufferSecurityCheck" }
staticruntime "On"
cppdialect "C++latest"
conformancemode "On"
warnings "Default"
floatingpoint "Fast"
characterset "Unicode"
exceptionhandling "Off"
editandcontinue "On"

-- Config for all 64-bit projects
filter( filter_64bit )
	architecture "x86_64"
filter {}

-- Config for Windows
filter "system:windows"
	buildoptions { "/utf-8", "/Zc:__cplusplus", "/Zc:preprocessor" }
	defines { "WIN32", "_WINDOWS", "_CRT_SECURE_NO_WARNINGS" }
filter {}

-- Config for Windows, release
filter { "system:windows", filter_rel_or_rtl }
	buildoptions { "/Gw" }
filter {}

-- Config for Linux
filter "system:linux"
	-- Fake headers for Linux
	includedirs { "thirdparty/linuxcompat" }
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
	"thirdparty/zlib/zconf.h",
	"thirdparty/zlib/zlib.h"
}

local zlib_sources = {
	"thirdparty/zlib/adler32.c",
	"thirdparty/zlib/compress.c",
	"thirdparty/zlib/crc32.c",
	"thirdparty/zlib/crc32.h",
	"thirdparty/zlib/deflate.c",
	"thirdparty/zlib/deflate.h",
	"thirdparty/zlib/gzclose.c",
	"thirdparty/zlib/gzguts.h",
	"thirdparty/zlib/gzlib.c",
	"thirdparty/zlib/gzread.c",
	"thirdparty/zlib/gzwrite.c",
	"thirdparty/zlib/infback.c",
	"thirdparty/zlib/inffast.c",
	"thirdparty/zlib/inffast.h",
	"thirdparty/zlib/inffixed.h",
	"thirdparty/zlib/inflate.c",
	"thirdparty/zlib/inflate.h",
	"thirdparty/zlib/inftrees.c",
	"thirdparty/zlib/inftrees.h",
	"thirdparty/zlib/trees.c",
	"thirdparty/zlib/trees.h",
	"thirdparty/zlib/uncompr.c",
	"thirdparty/zlib/zutil.c",
	"thirdparty/zlib/zutil.h"
}

local libpng_public = {
	"thirdparty/libpng/png.h",
	"thirdparty/libpng/pngconf.h"
}

local libpng_sources = {
	"thirdparty/libpng/png.c",
	"thirdparty/libpng/pngpriv.h",
	"thirdparty/libpng/pngstruct.h",
	"thirdparty/libpng/pnginfo.h",
	"thirdparty/libpng/pngdebug.h",
	"thirdparty/libpng/pngerror.c",
	"thirdparty/libpng/pngget.c",
	"thirdparty/libpng/pngmem.c",
	"thirdparty/libpng/pngpread.c",
	"thirdparty/libpng/pngread.c",
	"thirdparty/libpng/pngrio.c",
	"thirdparty/libpng/pngrtran.c",
	"thirdparty/libpng/pngrutil.c",
	"thirdparty/libpng/pngset.c",
	"thirdparty/libpng/pngtrans.c",
	"thirdparty/libpng/pngwio.c",
	"thirdparty/libpng/pngwrite.c",
	"thirdparty/libpng/pngwtran.c",
	"thirdparty/libpng/pngwutil.c"
}

local glew_public = {
	"thirdparty/glew/include/GL/glew.h",
	"thirdparty/glew/include/GL/wglew.h"
}

local glew_sources = {
	"thirdparty/glew/src/glew.c"
}

local xatlas_public = {
	"thirdparty/xatlas/xatlas.h",
}

local xatlas_sources = {
	"thirdparty/xatlas/xatlas.cpp",
}

local uvatlas_public = {
	"thirdparty/UVAtlas/UVAtlas/inc/UVAtlas.h",
}

local uvatlas_sources = {
	"thirdparty/UVAtlas/UVAtlas/geodesics/*.cpp",
	"thirdparty/UVAtlas/UVAtlas/geodesics/*.h",
	"thirdparty/UVAtlas/UVAtlas/isochart/*.cpp",
	"thirdparty/UVAtlas/UVAtlas/isochart/*.h"
}

local imgui_public = {
	"thirdparty/imgui/imgui.h",
	"thirdparty/imgui/imconfig.h"
}

local imgui_sources = {
	"thirdparty/imgui/imgui_internal.h",
	"thirdparty/imgui/imgui.cpp",
	"thirdparty/imgui/imgui_demo.cpp",
	"thirdparty/imgui/imgui_draw.cpp",
	"thirdparty/imgui/imgui_tables.cpp",
	"thirdparty/imgui/imgui_widgets.cpp",

	--"thirdparty/imgui/misc/freetype/imgui_freetype.*",

	"thirdparty/imgui/backends/imgui_impl_opengl3.*"
}

local meshoptimizer_public = {
	"thirdparty/meshoptimizer/src/meshoptimizer.h"
}

local meshoptimizer_sources = {
	"thirdparty/meshoptimizer/src/*.cpp",
}

-- freetype

local freetype_public_headers = {
	"thirdparty/freetype/include/ft2build.h",
	"thirdparty/freetype/include/freetype/*.h"
}

local freetype_config_headers = {
	"thirdparty/freetype/include/freetype/config/*.h"
}

local freetype_private_headers = {
	"thirdparty/freetype/include/freetype/internal/*.h"
}

local freetype_sources = {
	-- base
	"thirdparty/freetype/src/autofit/autofit.c",
	"thirdparty/freetype/src/base/ftbase.c",
	"thirdparty/freetype/src/base/ftbbox.c",
	"thirdparty/freetype/src/base/ftbdf.c",
	"thirdparty/freetype/src/base/ftbitmap.c",
	"thirdparty/freetype/src/base/ftcid.c",
	"thirdparty/freetype/src/base/ftfstype.c",
	"thirdparty/freetype/src/base/ftgasp.c",
	"thirdparty/freetype/src/base/ftglyph.c",
	"thirdparty/freetype/src/base/ftgxval.c",
	"thirdparty/freetype/src/base/ftinit.c",
	"thirdparty/freetype/src/base/ftmm.c",
	"thirdparty/freetype/src/base/ftotval.c",
	"thirdparty/freetype/src/base/ftpatent.c",
	"thirdparty/freetype/src/base/ftpfr.c",
	"thirdparty/freetype/src/base/ftstroke.c",
	"thirdparty/freetype/src/base/ftsynth.c",
	"thirdparty/freetype/src/base/fttype1.c",
	"thirdparty/freetype/src/base/ftwinfnt.c",
	"thirdparty/freetype/src/bdf/bdf.c",
	"thirdparty/freetype/src/bzip2/ftbzip2.c",
	"thirdparty/freetype/src/cache/ftcache.c",
	"thirdparty/freetype/src/cff/cff.c",
	"thirdparty/freetype/src/cid/type1cid.c",
	"thirdparty/freetype/src/gzip/ftgzip.c",
	"thirdparty/freetype/src/lzw/ftlzw.c",
	"thirdparty/freetype/src/pcf/pcf.c",
	"thirdparty/freetype/src/pfr/pfr.c",
	"thirdparty/freetype/src/psaux/psaux.c",
	"thirdparty/freetype/src/pshinter/pshinter.c",
	"thirdparty/freetype/src/psnames/psnames.c",
	"thirdparty/freetype/src/raster/raster.c",
	"thirdparty/freetype/src/sdf/sdf.c",
	"thirdparty/freetype/src/sfnt/sfnt.c",
	"thirdparty/freetype/src/smooth/smooth.c",
	"thirdparty/freetype/src/truetype/truetype.c",
	"thirdparty/freetype/src/type1/type1.c",
	"thirdparty/freetype/src/type42/type42.c",
	"thirdparty/freetype/src/winfonts/winfnt.c",

	"thirdparty/freetype/builds/windows/ftsystem.c",
	"thirdparty/freetype/builds/windows/ftdebug.c"
}

local fbxsdk_dir = os.getenv( "FBXSDK_DIR" )
local fbxsdk_include_dir = fbxsdk_dir .. "/include"
local fbxsdk_lib_dir = fbxsdk_dir .. "/lib/vs2019/x64"

include( "premake/premake-qt/qt.lua" )
local qt = premake.extensions.qt

-------------------------------------------------------------------------------

group "support"

project "core"
	kind "StaticLib"
	targetname "core"
	language "C++"
	includedirs { "thirdparty/mimalloc/include" }

	vpaths { ["code"] = "*" }

	files {
		"core/*.cpp",
		"core/*.h"
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

group "main"

project "engine"
	kind "WindowedApp"
	targetname "jaffaquake"
	language "C++"
	targetdir( out_dir )
	debugdir( out_dir )
	includedirs {
		"thirdparty/glew/include", "thirdparty/zlib", "thirdparty/libpng",
		"thirdparty/libpng_config", "thirdparty/imgui", "thirdparty/rapidjson/include"
	}
	defines { "Q_ENGINE", "GLEW_STATIC", "GLEW_NO_GLU", "IMGUI_USER_CONFIG=\"../../engine/client/q_imconfig.h\"" }
	links { "core", "meshoptimizer" }
	filter "system:windows"
		linkoptions { "/ENTRY:mainCRTStartup" }
		links {
			"shcore", "comctl32", "ws2_32", "dwmapi", "dsound", "dxguid", "opengl32", "noenv.obj", "zlib", "libpng"
		}
	filter {}
	filter "system:linux"
		links {
			"GL", "SDL2", "zlib", "png"
		}
	filter {}

	LinkToDistro()
	LinkToProfiler()

	disablewarnings { "4244", "4267" }

	files {
		"common/*",
		"resources/*",

		"framework/*",

		"engine/client/*",
		"engine/server/*",
		"engine/shared/*",

		"engine/mapedit/*",
		"engine/renderer/*",

		"game/client/cg_public.h",
		"game/server/g_public.h",

		zlib_public,

		libpng_public,

		imgui_public,
		imgui_sources,

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
			"**/*_win.*",
			"**/conproc.*",
			"resources/*"
		}
	filter {}

	removefiles {
		"engine/client/cl_console.cpp",
		"engine/client/cd_win.*",
		"**/cd_vorbis.cpp",
		"**.def",
		"**/*sv_null.*",
		"**_pch.cpp"
	}

project "cgame"
	kind "SharedLib"
	targetname "cgame"
	language "C++"
	targetdir "../game/base"
	links { "core" }

	disablewarnings { "4244", "4267" }

	pchsource( "game/client/cg_pch.cpp" )
	pchheader( "cg_local.h" )
	filter( "files:not game/client/**" )
		flags( { "NoPCH" } )
	filter( {} )

	files {
		"common/*",
		"game/client/*",
		"game/shared/*",
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

project "game"
	kind "SharedLib"
	targetname "game"
	language "C++"
	targetdir "../game/base"
	links { "core" }

	disablewarnings { "4244", "4311", "4302" }

	pchsource( "game/server/g_pch.cpp" )
	pchheader( "g_local.h" )
	filter( "files:not game/server/**" )
		flags( { "NoPCH" } )
	filter {}

	files {
		"common/*",
		"game/server/*",
		"game/shared/*",
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

-- Utils

if not _OPTIONS["exclude-utils"] then

	group "utilities"
	
	--[[
	project "d3dtest"
		kind "WindowedApp"
		targetname "d3dtest"
		language "C++"
		floatingpoint "Default"
		targetdir( out_dir )
		debugdir( out_dir )
		defines {  }
		sysincludedirs { "thirdparty/JoltPhysics/Jolt" }
		links { "core", "utils/d3dtest/joltlib/Jolt", "comctl32", "dxguid", "dxgi", "d3d11", "d3dcompiler" }
		
		filter "system:windows"
			linkoptions { "/ENTRY:mainCRTStartup" }
		filter {}
		
		files {
			"resources/*",

			"framework/*",

			"utils/d3dtest/**",
		}
	--]]
	
	project "stbtest"
		kind "ConsoleApp"
		targetname "stbtest"
		language "C++"
		floatingpoint "Default"
		targetdir( out_dir )
		debugdir( out_dir )

		files {
			"utils/stbtest/*"
		}

	--[[
	project "moduletest"
		kind "ConsoleApp"
		targetname "moduletest"
		language "C++"
		floatingpoint "Default"
		targetdir( out_dir )
		debugdir( out_dir )

		files {
			"utils/moduletest/*"
		}
	--]]

	project "mooned"
		kind "WindowedApp"
		targetname "mooned"
		language "C++"
		floatingpoint "Default"
		targetdir( out_dir )
		debugdir( out_dir )
		defines { "GLEW_STATIC", "GLEW_NO_GLU", "QT_DISABLE_DEPRECATED_BEFORE=QT_VERSION" }
		links { "core", "comctl32" }
		includedirs { "thirdparty/glew/include", "thirdparty/rapidjson/include", "thirdparty/glm", "utils/mooned" }
		
		filter "system:windows"
			linkoptions { "/ENTRY:mainCRTStartup" }
		filter {}
		
		pchsource( "utils/mooned/mooned_local.cpp" )
		pchheader( "mooned_local.h" )
		filter( "files:not utils/mooned/**" )
			flags( { "NoPCH" } )
		filter {}

		files {
			"resources/*",

			"framework/*",
			"framework/renderer/*",

			"utils/mooned/**",
			
			glew_public,
			glew_sources
		}
		
		qt.enable()
		
		qtprefix "Qt6"
		qtmain( false )
		
		filter( filter_dbg )
			qtsuffix "d"
		filter {}
		
		qtmodules { "core", "gui", "widgets", "opengl", "openglhack" }
		
	project "collisiontest"
		kind "WindowedApp"
		targetname "collisiontest"
		language "C++"
		floatingpoint "Default"
		targetdir( out_dir )
		debugdir( out_dir )
		includedirs { "thirdparty/glew/include", "utils/collisiontest/Jolt" }
		defines { "Q_CONSOLE_APP", "GLEW_STATIC", "GLEW_NO_GLU", "SDL_MAIN_HANDLED", "JPH_STAT_COLLECTOR", "JPH_PROFILE_ENABLED", "JPH_DEBUG_RENDERER", "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED" }
		links { "core", "opengl32", "comctl32", "utils/collisiontest/SDL2/lib/x64/SDL2", "utils/collisiontest/Jolt" }
		
		filter "system:windows"
			linkoptions { "/ENTRY:mainCRTStartup" }
		filter {}
		
		files {
			"resources/*",

			"framework/*",
			"framework/renderer/*",

			"utils/collisiontest/*",
			
			glew_public,
			glew_sources
		}

	project "qbsp4"
		kind "ConsoleApp"
		targetname "qbsp4"
		language "C++"
		floatingpoint "Default"
		targetdir( out_dir )
		debugdir( out_dir )
		defines { "Q_CONSOLE_APP" }
		links { "core" }
		includedirs { "utils/common2", "common" }

		files {
			"resources/windows_default.manifest",

			"common/*",

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
		targetdir( out_dir )
		debugdir( out_dir )
		defines { "Q_CONSOLE_APP" }
		links { "core" }
		includedirs { "utils/common2", "common" }

		files {
			"resources/windows_default.manifest",

			"common/*",

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
		targetdir( out_dir )
		debugdir( out_dir )
		defines { "Q_CONSOLE_APP" }
		links { "core" }
		includedirs { "utils/common2", "common", "thirdparty/stb" }

		files {
			"resources/windows_default.manifest",

			"common/*",

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

	--[[
	project "qatlas"
		kind "ConsoleApp"
		targetname "qatlas"
		language "C++"
		floatingpoint "Default"
		targetdir( out_dir )
		debugdir( out_dir )
		defines { "Q_CONSOLE_APP" }
		links { "core" }
		includedirs { "utils/common2", "thirdparty/xatlas" }

		files {
			"resources/windows_default.manifest",
			"common/q_formats.h",

			"utils/common2/cmdlib.*",

			"utils/qatlas/*",

			xatlas_public,
			xatlas_sources
		}
	--]]

	--[[
	project "qsmf"
		kind "ConsoleApp"
		targetname "qsmf"
		language "C++"
		floatingpoint "Default"
		targetdir( out_dir )
		debugdir( out_dir )
		defines { "Q_CONSOLE_APP" }
		links { "core", "zlib", "meshoptimizer" }
		includedirs { "utils/common2", "thirdparty/meshoptimizer/src", fbxsdk_include_dir }

		-- link to the FBX SDK
		filter( filter_dbg )
			links { fbxsdk_lib_dir .. "/debug/libfbxsdk-mt" }
			links { fbxsdk_lib_dir .. "/debug/libxml2-mt" }
		filter {}
		filter( filter_rel_or_rtl )
			links { fbxsdk_lib_dir .. "/release/libfbxsdk-mt" }
			links { fbxsdk_lib_dir .. "/release/libxml2-mt" }
		filter {}

		files {
			"resources/windows_default.manifest",
			"common/q_formats.h",

			"utils/common2/cmdlib.*",

			meshoptimizer_public,

			"utils/qsmf/*",
		}

		removefiles {
			"utils/qsmf/obj_reader.*"
		}
	--]]

	project "modelbuilder"
		kind "ConsoleApp"
		targetname "modelbuilder"
		language "C++"
		floatingpoint "Default"
		targetdir( out_dir )
		debugdir( out_dir )
		defines { "Q_CONSOLE_APP" }
		links { "core", "zlib", "meshoptimizer" }
		includedirs { "utils/common2", "thirdparty/meshoptimizer/src", "thirdparty/rapidjson/include", fbxsdk_include_dir }

		-- link to the FBX SDK
		filter( filter_dbg )
			links { fbxsdk_lib_dir .. "/debug/libfbxsdk-mt" }
			links { fbxsdk_lib_dir .. "/debug/libxml2-mt" }
		filter {}
		filter( filter_rel_or_rtl )
			links { fbxsdk_lib_dir .. "/release/libfbxsdk-mt" }
			links { fbxsdk_lib_dir .. "/release/libxml2-mt" }
		filter {}

		files {
			"resources/windows_default.manifest",
			"common/q_formats.h",

			"framework/*",

			"utils/common2/cmdlib.*",

			meshoptimizer_public,

			"utils/modelbuilder/*",
		}

end

group "thirdparty"

project "zlib"
	kind "StaticLib"
	targetname "zlib"
	language "C"
	defines { "_CRT_NONSTDC_NO_WARNINGS" }

	disablewarnings { "4267" }
	
	vpaths { ["code"] = "*" }

	files {
		zlib_public,
		zlib_sources
	}

project "libpng"
	kind "StaticLib"
	targetname "libpng"
	language "C"
	includedirs { "thirdparty/libpng_config", "thirdparty/zlib" }
	
	vpaths { ["code"] = "*" }

	files {
		libpng_public,
		libpng_sources,

		"thirdparty/libpng_config/pnglibconf.h"
	}

if not _OPTIONS["exclude-utils"] then

	project "meshoptimizer"
		kind "StaticLib"
		targetname "meshoptimizer"
		language "C++"

		vpaths { ["code"] = "*" }

		files {
			meshoptimizer_public,
			meshoptimizer_sources,
		}

end

--[[
project "freetype"
	kind "StaticLib"
	targetname "freetype"
	language "C"
	includedirs { "thirdparty/freetype/include", "thirdparty/libpng", "thirdparty/libpng_config", "thirdparty/zlib" }
	defines { "_CRT_NONSTDC_NO_WARNINGS", "FT2_BUILD_LIBRARY",
	"FT_CONFIG_OPTION_USE_PNG", "FT_CONFIG_OPTION_SUBPIXEL_RENDERING", "FT_CONFIG_OPTION_DISABLE_STREAM_SUPPORT",
	"FT_CONFIG_OPTION_SYSTEM_ZLIB" }

	files {
		freetype_public_headers,
		freetype_config_headers,
		freetype_private_headers,
		freetype_sources
	}
--]]

--[[
project "uvatlas"
	kind "StaticLib"
	targetname "uvatlas"
	language "C++"
	includedirs { "thirdparty/UVAtlas/UVAtlas", "thirdparty/UVAtlas/UVAtlas/inc", "thirdparty/UVAtlas/UVAtlas/geodesics" }

	disablewarnings { "4530" }

	files {
		uvatlas_public,
		uvatlas_sources
	}
--]]
