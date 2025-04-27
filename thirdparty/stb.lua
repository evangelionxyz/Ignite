project "STB"
kind "StaticLib"
language "C++"
cppdialect "C++20"
architecture "x64"
staticruntime "off"

targetdir (THIRDPARTY_OUTPUT_DIR)
objdir (INTOUTPUT_DIR)

files {
    "STB/stb_image.cpp"
}

includedirs {
    "STB/include"
}