project "JOLT"
kind "StaticLib"
language "C++"
cppdialect "C++17"
staticruntime "off"

targetdir (THIRDPARTY_OUTPUT_DIR)
objdir (INTOUTPUT_DIR)

files{
    "JOLT/Jolt/**.cpp",
}

includedirs{
    "JOLT/"
}

filter "system:windows"
    systemversion "latest"

filter "configurations:Debug"
    runtime "Debug"
    optimize "off"
    symbols "on"
    defines {
        "DEBUG"
    }

filter "configurations:Release"
    runtime "Release"
    optimize "on"
    symbols "off"
    defines {
        "NDEBUG"
    }

filter "configurations:Dist"
    runtime "Release"
    optimize "on"
    symbols "off"
    defines {
        "NDEBUG"
    }