add_executable(IGNITE
    core/vfs/vfs.cpp
    core/application.cpp
    core/device_manager.cpp
    core/logger.cpp
    core/main.cpp
    renderer/shader_factory.cpp
)

target_include_directories(IGNITE PRIVATE
    ${THIRDPARTY}/GLFW/src
    ${THIRDPARTY}/GLFW/include
    ${THIRDPARTY}/NVRHI/include
    ${THIRDPARTY}/SPDLOG/include
    ${THIRDPARTY}/GLM/
    ${THIRDPARTY}/ShaderMake/include
    ${VULKAN_SDK}/Include
    .
)

target_link_libraries(IGNITE glfw spdlog ShaderMakeBlob)

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