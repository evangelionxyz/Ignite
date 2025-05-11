project "YAMLCPP"
kind "StaticLib"
language "C++"
cppdialect "C++20"
staticruntime "off"
architecture "x64"

targetdir (THIRDPARTY_OUTPUT_DIR)
objdir (INTOUTPUT_DIR)

files {
    "YAML/src/**.cpp",
    "YAML/src/**.h",
    "YAML/include/yaml-cpp/**.h"
}

defines {
    "YAML_CPP_STATIC_DEFINE"
}

includedirs {
    "YAML/include/"
}

filter "system:linux"
pic "On"

filter "system:windows"
systemversion "latest"

filter "configurations:Debug"
runtime "Debug"
symbols "on"

filter "configurations:Release"
runtime "Release"
optimize "on"