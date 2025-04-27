project "NVRHI_VULKAN"
kind "StaticLib"
language "C++"
cppdialect "C++20"
staticruntime "off"
architecture "x64"

targetdir (THIRDPARTY_OUTPUT_DIR)
objdir (INTOUTPUT_DIR)

files {
    "NVRHI/include/nvrhi/vulkan.h",
    "NVRHI/src/vulkan/**.cpp",
    "NVRHI/src/vulkan/**.h",
}

includedirs {
    "NVRHI/include/",
    "NVRHI/thirdparty/Vulkan-Headers/include/",
}

defines {
    "NVRHI_WITH_VALIDATION",
    "NVRHI_WITH_VULKAN", 
    "VULKAN_HPP_STORAGE_SHARED",
    "VULKAN_HPP_STORAGE_SHARED_EXPORT",
}

filter "configurations:Debug"
    runtime "Debug"
    symbols "on"

filter "configurations:Release"
    runtime "Release"
    symbols "off"

--windows
filter "system:windows"
defines {
    "NOMINMAX",
    "VK_USE_PLATFORM_WIN32_KHR",
}
