require "buildtools/clangd"

workspace "cache"
    configurations {"debug", "release"}
    targetdir ("bin/%{cfg.buildcfg}/%{prj.name}")
	objdir ("bin/obj/%{cfg.buildcfg}/%{prj.name}")
    startproject "cache"


function enable_all_warnings()
    warnings "Extra"
    filter "action:gmake2"
        buildoptions { "-Wall", "-Wextra", "-Wpedantic" }
    filter {}
end

project "cache"
    kind "ConsoleApp"
    language "C"
    cdialect "C11"
    enable_all_warnings()

    files {
        "src/**.h",
        "src/**.c",
    }

    includedirs {
        "src/", 
    }

    filter "configurations:debug"
        symbols "On"

    filter "configurations:release"
        optimize "On"

