workspace "nvrhi"
    architecture "x64"
    configurations {
        "Debug",
        "Release"
    }

    flags { "MultiProcessorCompile "}

    startproject "IgniteEditor"

    BUILD_DIR = "%{wks.location}/bin"
    THIRDPARTY_DIR = "%{wks.location}/thirdparty"

    OUTPUT_DIR = "%{BUILD_DIR}/%{cfg.buildcfg}"
    INTOUTPUT_DIR = "%{wks.location}/bin-int/%{prj.name}"


