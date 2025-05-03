project "ShaderMake"
kind "StaticLib"
language "c++"
cppdialect "c++20"
staticruntime "off"

targetdir (THIRDPARTY_OUTPUT_DIR)
objdir (INTOUTPUT_DIR)

files {
    "ShaderMake/ShaderMake/src/argparse.c",
    "ShaderMake/ShaderMake/src/Compiler.cpp",
    "ShaderMake/ShaderMake/src/Context.cpp",
    "ShaderMake/ShaderMake/src/ShaderBlob.cpp",

    "ShaderMake/ShaderMake/include/ShaderMake/argparse.h",
    "ShaderMake/ShaderMake/include/ShaderMake/Compiler.h",
    "ShaderMake/ShaderMake/include/ShaderMake/Context.h",
    "ShaderMake/ShaderMake/include/ShaderMake/ShaderBlob.h",
    "ShaderMake/ShaderMake/include/ShaderMake/ShaderMake.h",
    "ShaderMake/ShaderMake/include/ShaderMake/Timer.h",
}

includedirs {
    "ShaderMake/ShaderMake/src",
    "ShaderMake/ShaderMake/include/ShaderMake",
}

defines {
    "SHADERMAKE_COLORS"
}

filter "system:windows"
defines {
    "WIN32_LEAN_AND_MEAN",
    "NOMINMAX",
    "_CRT_SECURE_NO_WARNINGS"
}
links {
    "d3dcompiler", "dxcompiler", "delayimp"
}

filter "system:linux"
links {
    "pthread"
}

filter "configurations:Debug"
runtime "Debug"
symbols "on"

filter "configurations:Release"
runtime "Release"
symbols "off"

