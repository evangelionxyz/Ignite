project "NVRHI"
kind "StaticLib"
language "C++"
cppdialect "C++20"
staticruntime "off"
architecture "x64"

targetdir (THIRDPARTY_OUTPUT_DIR)
objdir (INTOUTPUT_DIR)

files {
    "NVRHI/src/common/format-info.cpp",
    "NVRHI/src/common/misc.cpp",
    "NVRHI/src/common/sparse-bitset.cpp",
    "NVRHI/src/common/state-tracking.cpp",
    "NVRHI/src/common/utils.cpp",
    "NVRHI/src/common/aftermath.cpp",

    "NVRHI/src/common/sparse-bitset.h",
    "NVRHI/src/common/state-tracking.h",

    "NVRHI/include/nvrhi.h",
    "NVRHI/include/utils.h",
    "NVRHI/include/validation.h",
    "NVRHI/include/vulkan.h",

    -- validation layer
    "NVRHI/src/validation/validation-backend.h",
    "NVRHI/src/validation/validation-commandlist.cpp",
    "NVRHI/src/validation/validation-device.cpp",
}

includedirs {
    "NVRHI/include/",
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
}
files {
    "NVRHI/include/common/nvrhiHLSL.h",
}

