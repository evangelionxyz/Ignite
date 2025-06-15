workspace "IGN"
architecture "x64"
configurations {
    "Debug",
    "Release",
    "Dist"
}

flags { "MultiProcessorCompile" }

startproject "IgniteEditor"

BUILD_DIR = "%{wks.location}/bin"
OUTPUT_DIR = "%{BUILD_DIR}/%{cfg.buildcfg}"

THIRDPARTY_DIR = "%{wks.location}/thirdparty"
THIRDPARTY_OUTPUT_DIR = "%{BUILD_DIR}/%{cfg.buildcfg}/thirdparty/%{prj.name}"

INTOUTPUT_DIR = "%{wks.location}/bin/objs/%{cfg.buildcfg}/%{prj.name}"

include "thirdparty/thirdparty.lua"

group "Engine"
    include "editor/ignite-editor.lua"
    include "engine/ignite-engine.lua"
    include "scriptcore/ignite-script.lua"
group ""
