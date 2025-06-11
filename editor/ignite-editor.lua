project "IgniteEditor"
kind "ConsoleApp"
staticruntime "off"
architecture "x64"
language "c++"
cppdialect "c++20"

targetdir (OUTPUT_DIR)
objdir (INTOUTPUT_DIR)

files {
    "src/**.cpp",
    "src/**.hpp",
    "src/**.h",
}

links {
    "IgniteEngine"
}

includedirs {
    "src",
    "%{wks.location}/engine/ignite/src",
    "%{IncludeDir.GLFW}",
    "%{IncludeDir.BOX2D}",
    "%{IncludeDir.ENTT}",
    "%{IncludeDir.JOLT}",
    "%{IncludeDir.GLM}",
    "%{IncludeDir.IMGUI}",
    "%{IncludeDir.FMOD}",
    "%{IncludeDir.IMGUIZMO}",
    "%{IncludeDir.SPDLOG}",
    "%{IncludeDir.NVRHI}",
    "%{IncludeDir.STB}",
    "%{IncludeDir.NVRHI_VULKAN_HPP}",
    "%{IncludeDir.VULKAN_SDK}",
    "%{IncludeDir.SHADERMAKE}",
    "%{IncludeDir.ASSIMP}",
    "%{IncludeDir.YAMLCPP}",
}

defines {
    "SHADERMAKE_COLORS",
    "YAML_CPP_STATIC_DEFINE",
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

--linux

--windows

filter "system:windows"
buildoptions {
    "/utf-8"
}
defines {
    "PLATFORM_WINDOWS",
    "GLFW_EXPOSE_NATIVE_WIN32",
    "IGNITE_WITH_DX12",
    "IGNITE_WITH_VULKAN",
    "NOMINMAX",
    "_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING",
    "_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS",
    "_CRT_SECURE_NO_WARNINGS"
}

filter "configurations:Debug"
runtime "Debug"
symbols "on"

filter "configurations:Debug"
    runtime "Debug"
    optimize "off"
    symbols "on"
    defines {
        "DEBUG",
        "_DEBUG",
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