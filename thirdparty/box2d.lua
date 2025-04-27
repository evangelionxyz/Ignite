project "BOX2D"
kind "StaticLib"
language "C"
cdialect "C17"
staticruntime "off"
architecture "x64"

targetdir (THIRDPARTY_OUTPUT_DIR)
objdir (INTOUTPUT_DIR)

files {
    "BOX2D/src/**.c",
}

includedirs {
    "BOX2D/include",
}

defines {
    "BOX2D_ENABLE_SIMD",
}

--linux
filter "system:linux"
pic "on"

--windows
filter "system:windows"
systemversion "latest"
buildoptions {
    "/experimental:c11atomics",
}

filter "configurations:Ddebug"
runtime "debug"
symbols "on"

filter "configurations:Release"
runtime "release"
symbols "off"
optimize "on"