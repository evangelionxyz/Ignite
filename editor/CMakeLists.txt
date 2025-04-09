add_executable(IgniteEditor
    src/editor_layer.cpp
    src/editor_app.cpp
    src/scene_panel.cpp
    src/render_target.cpp
)

target_include_directories(IgniteEditor PRIVATE
    src
    ${IGNITE_DIR}/src

    ${THIRDPARTY}/GLFW/src
    ${THIRDPARTY}/GLFW/include
    ${THIRDPARTY}/NVRHI/include
    ${THIRDPARTY}/SPDLOG/include
    ${THIRDPARTY}/GLM
    ${THIRDPARTY}/IMGUI
    ${THIRDPARTY}/STB/include
    ${THIRDPARTY}/ShaderMake/include
    ${VULKAN_SDK}/Include
)

target_link_libraries(IgniteEditor IgniteEngine)

set(IGNITE_WITH_DX12 ON)
set(IGNITE_WITH_DX11 OFF)

if (WIN32)
  add_definitions(-DPLATFORM_WINDOWS)
  add_definitions(-DNOMINMAX)

  if (IGNITE_WITH_DX12)
    target_compile_definitions(IgniteEditor PRIVATE IGNITE_WITH_DX12)
  endif()

endif()

if (IGNITE_WITH_VULKAN)
  if (NOT NVRHI_BUILD_SHARED)
    target_link_libraries(IgniteEditor nvrhi_vk)
  endif()
  target_sources(IgniteEditor PRIVATE core/device_manager_vulkan.cpp)
endif()
target_link_libraries(IgniteEditor nvrhi) # needs to come after nvrhi_d3d12 etc.

target_compile_definitions(IgniteEditor PRIVATE
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:RELEASE_BUILD>
    #IGNITE_WITH_DX12
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET IgniteEditor PROPERTY CXX_STANDARD 23)
endif()