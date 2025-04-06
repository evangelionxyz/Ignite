add_executable(IGNITE
    editor/main.cpp
    editor/ignite_editor_app.cpp
    editor/scene_render_pass.cpp
    editor/imgui_render_pass.cpp
    editor/scene_panel.cpp

    core/vfs/vfs.cpp
    core/application.cpp
    core/layer.cpp
    core/irender_pass.cpp
    core/device_manager.cpp
    core/logger.cpp
    core/imgui/imgui_nvrhi.cpp
    core/imgui/imgui_renderer.cpp
    
    renderer/shader_factory.cpp
    scene/scene.cpp
)

target_include_directories(IGNITE PRIVATE
    ${THIRDPARTY}/GLFW/src
    ${THIRDPARTY}/GLFW/include
    ${THIRDPARTY}/NVRHI/include
    ${THIRDPARTY}/SPDLOG/include
    ${THIRDPARTY}/GLM
    ${THIRDPARTY}/IMGUI
    ${THIRDPARTY}/ShaderMake/include
    ${VULKAN_SDK}/Include
    .
)

target_link_libraries(IGNITE glfw spdlog ShaderMakeBlob imgui)

set(IGNITE_WITH_DX12 ON)
set(IGNITE_WITH_DX11 OFF)

if (WIN32)
  add_definitions(-DPLATFORM_WINDOWS)
  add_definitions(-DNOMINMAX)

  if (IGNITE_WITH_DX12)
    target_compile_definitions(IGNITE PRIVATE IGNITE_WITH_DX12)
    target_sources(IGNITE PRIVATE core/device_manager_dx12.cpp)
    if (NOT NVRHI_BUILD_SHARED)
      target_link_libraries(IGNITE nvrhi_d3d12)
    endif()
    target_link_libraries(IGNITE d3d12 dxgi d3dcompiler dxguid)
  endif()

endif()

if (IGNITE_WITH_VULKAN)
  if (NOT NVRHI_BUILD_SHARED)
    target_link_libraries(IGNITE nvrhi_vk)
  endif()
  target_sources(IGNITE PRIVATE core/device_manager_vulkan.cpp)
endif()
target_link_libraries(IGNITE nvrhi) # needs to come after nvrhi_d3d12 etc.

target_compile_definitions(IGNITE PRIVATE
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:RELEASE_BUILD>
    #IGNITE_WITH_DX12
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET IGNITE PROPERTY CXX_STANDARD 23)
endif()