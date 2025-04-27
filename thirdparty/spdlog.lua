project "SPDLOG"
kind "StaticLib"
language "C++"
cppdialect "C++20"
staticruntime "off"
architecture "x64"

targetdir (THIRDPARTY_OUTPUT_DIR)
objdir (INTOUTPUT_DIR)

files {
    "SPDLOG/src/async.cpp",
    "SPDLOG/src/bundled_fmtlib_format.cpp",
    "SPDLOG/src/cfg.cpp",
    "SPDLOG/src/color_sinks.cpp",
    "SPDLOG/src/file_sinks.cpp",
    "SPDLOG/src/spdlog.cpp",
    "SPDLOG/src/stdout_sinks.cpp",
}

includedirs {
    "SPDLOG/include"
}

defines {
    "SPDLOG_COMPILED_LIB",
}

--windows
filter "system:windows"
defines {
    "WIN32", "_WINDOWS", "_UNICODE"
}
buildoptions {
    "/utf-8", "/interface"
}

filter "configurations:Debug"
runtime "debug"
symbols "on"

filter "configurations:Release"
runtime "release"
symbols "off"
optimize "on"
