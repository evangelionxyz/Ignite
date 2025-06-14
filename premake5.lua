workspace "IGN"
architecture "x64"
configurations {
    "Debug",
    "Release"
}

flags { 
    "MultiProcessorCompile"
}

--startproject "IgniteEditor"

BUILD_DIR = "%{wks.location}/bin"
THIRDPARTY_DIR = "%{wks.location}/thirdparty"

OUTPUT_DIR = "%{BUILD_DIR}/%{cfg.buildcfg}"
THIRDPARTY_OUTPUT_DIR = "%{BUILD_DIR}/%{cfg.buildcfg}/thirdparty/%{prj.name}"
INTOUTPUT_DIR = "%{wks.location}/bin-int/%{prj.name}"

include "thirdparty/thirdparty.lua"

group "Engine"
    include "editor/ignite-editor.lua"
    include "engine/ignite/ignite-engine.lua"
    include "scriptcore/script-core.lua"
group ""
