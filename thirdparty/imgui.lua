project "IMGUI"
kind "StaticLib"
language "C++"
cppdialect "C++17"
staticruntime "off"
architecture "x64"

targetdir (THIRDPARTY_OUTPUT_DIR)
objdir (INTOUTPUT_DIR)

files {
    "IMGUI/imgui_demo.cpp",
    "IMGUI/imgui_draw.cpp",
    "IMGUI/imgui_tables.cpp",
    "IMGUI/imgui_widgets.cpp",
    "IMGUI/imgui.cpp",
    "IMGUI/backends/imgui_impl_glfw.cpp",
    "IMGUI/backends/imgui_impl_vulkan.cpp",
    "IMGUI/backends/imgui_impl_vulkan.h",
    "IMGUI/imconfig.h",
    "IMGUI/imgui.h",
    "IMGUI/imgui_internal.h",
    "IMGUI/imstb_rectpack.h",
    "IMGUI/imstb_textedit.h",
    "IMGUI/imstb_truetype.h",

    -- include imguizmo src to compile
    "IMGUIZMO/ImGuizmo.cpp",
    "IMGUIZMO/ImGradient.cpp",
    "IMGUIZMO/GraphEditor.cpp",
    "IMGUIZMO/ImCurveEdit.cpp",
    "IMGUIZMO/ImSequencer.cpp",
}

includedirs {
    "IMGUI/",
    "GLFW/include",
    "%{IncludeDir.NVRHI_VULKAN_HPP}",
}

--windows
filter "system:windows"
systemversion "latest"
files {
    "IMGUI/backends/imgui_impl_dx12.cpp",
    "IMGUI/backends/imgui_impl_dx12.hpp",
}