project "NVRHI_D3D12"
kind "StaticLib"
language "C++"
cppdialect "C++20"
staticruntime "off"
architecture "x64"

targetdir (THIRDPARTY_OUTPUT_DIR)
objdir (INTOUTPUT_DIR)

files {
    "NVRHI/include/nvrhi/d3d12.h",
    "NVRHI/src/common/dxgi-format.h",
    "NVRHI/src/common/dxgi-format.cpp",
    "NVRHI/src/d3d12/**.cpp",
    "NVRHI/src/d3d12/**.h",
}

includedirs {
    "NVRHI/include/",
}

defines {
    "NVRHI_WITH_VALIDATION",
    "NVRHI_WITH_DX12",
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