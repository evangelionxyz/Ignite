set(imgui_srcs
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/imconfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/backends/imgui_impl_vulkan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/imgui_internal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/imgui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/imstb_rectpack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/imstb_textedit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/imstb_truetype.h
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/backends/imgui_impl_vulkan.cpp

    # IMGUIZMO
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUIZMO/GraphEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUIZMO/ImCurveEdit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUIZMO/ImGradient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUIZMO/ImGuizmo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUIZMO/ImSequencer.cpp
)

add_library(imgui STATIC ${imgui_srcs})

if (WIN32)
    target_sources(imgui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/backends/imgui_impl_win32.cpp)
    target_sources(imgui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI/backends/imgui_impl_dx12.cpp)
endif()

target_include_directories(imgui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/IMGUI
    ${CMAKE_CURRENT_SOURCE_DIR}/GLFW/include
    ${CMAKE_CURRENT_SOURCE_DIR}/NVRHI/thirdparty/Vulkan-Headers/include
)