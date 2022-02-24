workspace "fenrir" 
    configurations { "debug", "release" }
    startproject "fenrir"

project "fenrir"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin"

    files {
        "src/**.cpp",
        "src/**.h"
    }

    includedirs {
        "vendor/glfw/include",
        "vendor/spdlog/include",
        "src"
    }

    links {
        "GLFW",
        "pthread",
        "dl",
        "vulkan"
    }

    filter "configurations:debug"
        defines { "FEN_DEBUG" }
        symbols "On"

    filter "configurations:release"
        defines { "FEN_RELEASE" }
        optimize "On"

include "vendor/glfw"

