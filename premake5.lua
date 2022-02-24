workspace "fenrir" 
    configurations { "Debug", "Release" }
    startproject "fenrir"

project "fenrir"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"

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

include "vendor/glfw"

