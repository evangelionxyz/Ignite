VULKAN_SDK_PATH = os.getenv("VULKAN_SDK")

--includedirs
IncludeDir = {}
IncludeDir["GLFW"] = "%{THIRDPARTY_DIR}/GLFW/include"
IncludeDir["BOX2D"] = "%{THIRDPARTY_DIR}/BOX2D/include"
IncludeDir["ENTT"] = "%{THIRDPARTY_DIR}/entt/"
IncludeDir["GLM"] = "%{THIRDPARTY_DIR}/GLM/"
IncludeDir["IMGUI"] = "%{THIRDPARTY_DIR}/IMGUI/"
IncludeDir["IMGUIZMO"] = "%{THIRDPARTY_DIR}/IMGUIZMO/"
IncludeDir["NVRHI"] = "%{THIRDPARTY_DIR}/NVRHI/include"
IncludeDir["SPDLOG"] = "%{THIRDPARTY_DIR}/SPDLOG/include"
IncludeDir["STB"] = "%{THIRDPARTY_DIR}/STB/include"
IncludeDir["VULKAN_SDK"] = "%{VULKAN_SDK_PATH}/Include"
IncludeDir["NVRHI_VULKAN_HPP"] = "%{THIRDPARTY_DIR}/NVRHI/thirdparty/Vulkan-Headers/include"
IncludeDir["SHADERMAKE"] = "%{THIRDPARTY_DIR}/ShaderMake/ShaderMake/include"
IncludeDir["ASSIMP"] = "%{THIRDPARTY_DIR}/ASSIMP/include"
IncludeDir["YAMLCPP"] = "%{THIRDPARTY_DIR}/YAML/include"

--library dirs
LibraryDir = {}
LibraryDir["VULKAN_SDK"] = "%{VULKAN_SDK_PATH}/Lib"

--libraries
Library               = {}
Library["winsock"]    = "ws2_32.lib"
Library["winmm"]      = "winmm.lib"
Library["winversion"] = "version.lib"
Library["bcrypt"]     = "bcrypt.lib"
Library["vulkan"]     = "%{LibraryDir.VULKAN_SDK}/vulkan-1.lib"

Library["ShaderC_Debug"]               = "%{LibraryDir.VULKAN_SDK}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"]           = "%{LibraryDir.VULKAN_SDK}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"]      = "%{LibraryDir.VULKAN_SDK}/spirv-cross-glsld.lib"
Library["SPIRV_Cross_HLSL_Debug"]      = "%{LibraryDir.VULKAN_SDK}/spirv-cross-hlsld.lib"
Library["SPIRV_Cross_Reflect_Debug"]   = "%{LibraryDir.VULKAN_SDK}/spirv-cross-reflectd.lib"
Library["SPIRV_Cross_Util_Debug"]      = "%{LibraryDir.VULKAN_SDK}/spirv-cross-utild.lib"
Library["SPIRV_Tools_Debug"]           = "%{LibraryDir.VULKAN_SDK}/SPIRV-Toolsd.lib"

Library["ShaderC"]                     = "%{LibraryDir.VULKAN_SDK}/shaderc_shared.lib"
Library["SPIRV_Cross"]                 = "%{LibraryDir.VULKAN_SDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL"]            = "%{LibraryDir.VULKAN_SDK}/spirv-cross-glsl.lib"
Library["SPIRV_Cross_HLSL"]            = "%{LibraryDir.VULKAN_SDK}/spirv-cross-hlsl.lib"
Library["SPIRV_Cross_Reflect"]         = "%{LibraryDir.VULKAN_SDK}/spirv-cross-reflect.lib"
Library["SPIRV_Cross_Util"]            = "%{LibraryDir.VULKAN_SDK}/spirv-cross-util.lib"
Library["SPIRV_Tools"]                 = "%{LibraryDir.VULKAN_SDK}/SPIRV-Tools.lib"

-- include lua files
group "Third Party"
include "box2d.lua"
include "glfw.lua"
include "spdlog.lua"
include "imgui.lua"
include "stb.lua"
include "assimp.lua"
include "yaml-cpp.lua"
group ""

group "NVIDIA"
include "nvrhi.lua"
include "nvrhi-vk.lua"
include "nvrhi-d3d12.lua"
include "shadermake.lua"
group ""
