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
    "%{IncludeDir.GLM}",
    "%{IncludeDir.IMGUI}",
    "%{IncludeDir.IMGUIZMO}",
    "%{IncludeDir.SPDLOG}",
    "%{IncludeDir.NVRHI}",
    "%{IncludeDir.STB}",
    "%{IncludeDir.NVRHI_VULKAN_HPP}",
    "%{IncludeDir.VULKAN_SDK}",
    "%{IncludeDir.SHADERMAKE}",
    "%{IncludeDir.ASSIMP}",
}

defines {
    "SHADERMAKE_COLORS"
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

filter "configurations:Release"
runtime "Release"
symbols "off"