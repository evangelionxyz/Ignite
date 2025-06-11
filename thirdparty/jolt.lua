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

defines {
    "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
    "JPH_DEBUG_RENDERER",
    "JPH_PROFILE_ENABLED",
    "JPH_OBJECT_STREAM",
    "JPH_USE_AVX2",
    "JPH_USE_AVX",
    "JPH_USE_SSE4_1",
    "JPH_USE_SSE4_2",
    "JPH_USE_LZCNT",
    "JPH_USE_TZCNT",
    "JPH_USE_F16C",
    "JPH_USE_FMADD",
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
        "_WINDOWS",
        "_DEBUG",
    }

filter "configurations:Release"
    runtime "Release"
    optimize "on"
    symbols "on"
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