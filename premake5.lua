workspace "fenrir"
	configurations { "Debug", "Release" }
	architecture "x86_64"
	startproject "fenrir"
	flags {
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "ghua/vendor/glfw"
include "ghua"
include "fenrir"