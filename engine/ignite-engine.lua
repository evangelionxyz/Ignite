project "IgniteEngine"
kind "StaticLib"
language "C++"
cppdialect "C++23"

targetdir (OUTPUT_DIR)
objdir (INTOUTPUT_DIR)

files {
    "src/ignite/**.cpp",
    "src/ignite/**.hpp",
    "src/ignite/**.h",
}

includedirs {
    "src",
    "%{IncludeDir.GLFW}",
    "%{IncludeDir.BOX2D}",
    "%{IncludeDir.ENTT}",
    "%{IncludeDir.FMOD}",
    "%{IncludeDir.GLM}",
    "%{IncludeDir.JOLT}",
    "%{IncludeDir.IMGUI}",
    "%{IncludeDir.IMGUIZMO}",
    "%{IncludeDir.SPDLOG}",
    "%{IncludeDir.MONO}",
    "%{IncludeDir.NVRHI}",
    "%{IncludeDir.STB}",
    "%{IncludeDir.NVRHI_VULKAN_HPP}",
    "%{IncludeDir.VULKAN_SDK}",
    "%{IncludeDir.FILEWATCHER}",
    "%{IncludeDir.SHADERMAKE}",
    "%{IncludeDir.ZLIB}",
    "%{IncludeDir.ASSIMP}",
    "%{IncludeDir.YAMLCPP}",
}

links {
    "IMGUI",
    "BOX2D",
    "GLFW",
    "STB",
    "JOLT",
    "SPDLOG",
    "ShaderMake",
    "ASSIMP",
    "NVRHI",
    "NVRHI_VULKAN",
    "ZLIB",
    "YAMLCPP",
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
filter "system:linux"
pic "on"
defines {
    "PLATFORM_LINUX",
    "_GLFW_X11",
}
libdirs {
    "/usr/lib"
}
includedirs {
    "/usr/include"
}

links {
    "vulkan",
    "shaderc_shared",
    "spirv-cross-core",
    "spirv-cross-glsl",
    "pthread",
    "dl",
    "m",
    "rt",
    "glib-2.0"
}


--windows
filter "system:windows"
    systemversion "latest"
    buildoptions {
        "/utf-8"
    }
    links {
        "d3d12.lib",
        "dxgi.lib",
        "d3dcompiler",
        "dxguid",

        "NVRHI_D3D12",
        "NVRHI_VULKAN",

        "%{Library.winmm}",
        "%{Library.winsock}",
        "%{Library.winversion}",
        "%{Library.bcrypt}",
        "%{Library.vulkan}",
        "%{Library.mono}",
        "%{Library.FMOD}",
    }
    defines {
        "PLATFORM_WINDOWS",
        "GLFW_EXPOSE_NATIVE_WIN32",
        "NOMINMAX",
        "IGNITE_WITH_DX12",
        "IGNITE_WITH_VULKAN",
        "_CRT_SECURE_NO_WARNINGS",
        "WIN32",
        "_WINDOWS"
    }

    postbuildcommands {
        '{COPYFILE} "%{THIRDPARTY_DIR}/FMOD/lib/windows/x64/fmod.dll" "%{cfg.targetdir}"'    
    }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        defines {
            "DEBUG",
            "_DEBUG"
        }
        links {
            "%{Library.ShaderC_Debug}",
            "%{Library.SPIRV_Cross_Debug}",
            "%{Library.SPIRV_Cross_GLSL_Debug}",
            "%{Library.SPIRV_Cross_HLSL_Debug}",
            "%{Library.SPIRV_Cross_Reflect_Debug}",
            "%{Library.SPIRV_Cross_Util_Debug}",
            "%{Library.SPIRV_Tools_Debug}",
        }
    filter "configurations:Release"
        runtime "release"
        optimize "on"
        symbols "on" -- with debug info
        defines {
            "NDEBUG"
        }
        links {
            "%{Library.ShaderC}",
            "%{Library.SPIRV_Cross}",
            "%{Library.SPIRV_Cross_GLSL}",
            "%{Library.SPIRV_Cross_HLSL}",
            "%{Library.SPIRV_Cross_Reflect}",
            "%{Library.SPIRV_Cross_Util}",
            "%{Library.SPIRV_Tools}",
        }

    filter "configurations:Dist"
        runtime "release"
        optimize "on"
        symbols "off" -- without debug info
        defines {
            "NDEBUG"
        }
        links {
            "%{Library.ShaderC}",
            "%{Library.SPIRV_Cross}",
            "%{Library.SPIRV_Cross_GLSL}",
            "%{Library.SPIRV_Cross_HLSL}",
            "%{Library.SPIRV_Cross_Reflect}",
            "%{Library.SPIRV_Cross_Util}",
            "%{Library.SPIRV_Tools}",
        }
