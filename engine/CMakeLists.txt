add_library(IgniteEngine
    src/ignite/core/vfs/vfs.cpp
    src/ignite/core/application.cpp

    src/ignite/core/device/device_manager.cpp

    src/ignite/core/logger.cpp
    src/ignite/core/imgui/imgui_nvrhi.cpp
    src/ignite/core/imgui/imgui_layer.cpp
    src/ignite/graphics/shader_factory.cpp
    src/ignite/scene/scene.cpp
    src/ignite/graphics/window.cpp
    src/ignite/core/device/device_manager_vk.cpp
    src/ignite/core/device/device_manager_vk.hpp
)

target_include_directories(IgniteEngine PRIVATE
    src/
    src/ignite
    ${THIRDPARTY}/GLFW/src
    ${THIRDPARTY}/GLFW/include
    ${THIRDPARTY}/NVRHI/include
    ${THIRDPARTY}/SPDLOG/include
    ${THIRDPARTY}/GLM
    ${THIRDPARTY}/IMGUI
    ${THIRDPARTY}/ShaderMake/include
    ${VULKAN_SDK}/Include
)

target_link_libraries(IgniteEngine glfw spdlog ShaderMakeBlob imgui)

set(IGNITE_WITH_DX12 ON)
set(IGNITE_WITH_DX11 OFF)

if (WIN32)
  add_definitions(-DPLATFORM_WINDOWS)
  add_definitions(-DNOMINMAX)

  if (IGNITE_WITH_DX12)
    target_compile_definitions(IgniteEngine PRIVATE IGNITE_WITH_DX12)
    target_sources(IgniteEngine PRIVATE src/ignite/core/device/device_manager_dx12.cpp)
    if (NOT NVRHI_BUILD_SHARED)
      target_link_libraries(IgniteEngine nvrhi_d3d12)
    endif()
    target_link_libraries(IgniteEngine d3d12 dxgi d3dcompiler dxguid)
  endif()

endif()

if (IGNITE_WITH_VULKAN)
  if (NOT NVRHI_BUILD_SHARED)
    target_link_libraries(IgniteEngine nvrhi_vk)
  endif()
  target_sources(IgniteEngine PRIVATE core/device_manager_vulkan.cpp)
endif()
target_link_libraries(IgniteEngine nvrhi) # needs to come after nvrhi_d3d12 etc.

target_compile_definitions(IgniteEngine PRIVATE
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:RELEASE_BUILD>
    #IGNITE_WITH_DX12
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET IgniteEngine PROPERTY CXX_STANDARD 23)
endif()