add_library(IgniteEngine
  src/ignite/asset/asset.cpp
  src/ignite/core/vfs/vfs.cpp
  src/ignite/core/application.cpp
  src/ignite/core/device/device_manager.cpp
  src/ignite/core/device/device_manager_vk.cpp
  src/ignite/core/logger.cpp
  src/ignite/core/command.cpp
  src/ignite/core/uuid.cpp
  src/ignite/core/input/input.cpp
  src/ignite/imgui/imgui_nvrhi.cpp
  src/ignite/imgui/imgui_layer.cpp
  src/ignite/graphics/shader_factory.cpp
  src/ignite/graphics/shader.cpp
  src/ignite/graphics/texture.cpp
  src/ignite/graphics/window.cpp
  src/ignite/graphics/renderer_2d.cpp
  src/ignite/graphics/renderer.cpp
  src/ignite/math/math.cpp
  src/ignite/scene/entity.cpp
  src/ignite/scene/scene.cpp
  src/ignite/scene/camera.cpp
  src/ignite/scene/scene_manager.cpp
  src/ignite/physics/2d/physics_2d.cpp
  src/ignite/physics/2d/physics_2d_component.hpp
  src/ignite/project/project.cpp
)

#target_precompile_headers(IgniteEngine PRIVATE src/ignite/pch.hpp)

target_include_directories(IgniteEngine PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/
  ${THIRDPARTY}/GLFW/src
  ${THIRDPARTY}/GLFW/include
  ${THIRDPARTY}/BOX2D/include
  ${THIRDPARTY}/NVRHI/include
  ${THIRDPARTY}/SPDLOG/include
  ${THIRDPARTY}/GLM
  ${THIRDPARTY}/IMGUI
  ${THIRDPARTY}/IMGUIZMO
  ${THIRDPARTY}/entt
  ${THIRDPARTY}/STB/include
  ${THIRDPARTY}/ShaderMake/include
  ${THIRDPARTY}/NVRHI/thirdparty/Vulkan-Headers/include
  ${VULKAN_INCLUDE_DIR}
)

target_link_libraries(IgniteEngine PUBLIC glfw spdlog ShaderMakeBlob imgui stblib box2d)

if (MSVC)
    #target_compile_options(IgniteEngine PRIVATE "/utf-8")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # GCC and Clang support -finput-charset=utf-8
    #add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

if (WIN32)
  target_compile_definitions(IgniteEngine PRIVATE 
    PLATFORM_WINDOWS
    NOMINMAX
    WIN32_LEAN_AND_MEAN
  )

  # DirectX 12 Build
  if (IGNITE_WITH_DX12)
    message(">> Ignite Engine: Build with DX12")

    target_sources(IgniteEngine PRIVATE src/ignite/core/device/device_manager_dx12.cpp)
    
    target_compile_definitions(IgniteEngine PRIVATE IGNITE_WITH_DX12)
    target_link_libraries(IgniteEngine PUBLIC nvrhi_d3d12 d3d12 dxgi d3dcompiler dxguid)
  endif()

  # Vulkan Build
  if (IGNITE_WITH_VULKAN)
    message(">> Ignite Engine: Build with Vulkan")
    target_link_directories(IgniteEngine PRIVATE ${VULKAN_LIBRARY_DIR})
    target_link_libraries(IgniteEngine PUBLIC nvrhi_vk)

    target_compile_definitions(IgniteEngine PRIVATE IGNITE_WITH_VULKAN)

    target_sources(IgniteEngine PRIVATE src/ignite/core/device/device_manager_vk.cpp)

    # build type specific
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_link_libraries(IgniteEngine PUBLIC
        ${VULKAN_LIBRARY_DIR}/vulkan-1.lib
        ${VULKAN_LIBRARY_DIR}/shaderc_sharedd.lib
        ${VULKAN_LIBRARY_DIR}/SPIRVd.lib
        ${VULKAN_LIBRARY_DIR}/spirv-cross-cored.lib
        ${VULKAN_LIBRARY_DIR}/spirv-cross-glsld.lib
        ${VULKAN_LIBRARY_DIR}/spirv-cross-hlsld.lib
        ${VULKAN_LIBRARY_DIR}/spirv-cross-reflectd.lib
        ${VULKAN_LIBRARY_DIR}/spirv-cross-utild.lib
        ${VULKAN_LIBRARY_DIR}/SPIRV-Toolsd.lib
      )
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
      target_link_libraries(IgniteEngine PUBLIC
        ${VULKAN_LIBRARY_DIR}/vulkan-1.lib
        ${VULKAN_LIBRARY_DIR}/shaderc_shared.lib
        ${VULKAN_LIBRARY_DIR}/SPIRV.lib
        ${VULKAN_LIBRARY_DIR}/spirv-cross-core.lib
        ${VULKAN_LIBRARY_DIR}/spirv-cross-glsl.lib
        ${VULKAN_LIBRARY_DIR}/spirv-cross-hlsl.lib
        ${VULKAN_LIBRARY_DIR}/spirv-cross-reflect.lib
        ${VULKAN_LIBRARY_DIR}/spirv-cross-util.lib
        ${VULKAN_LIBRARY_DIR}/SPIRV-Tools.lib
      )
    endif()
  endif()

elseif(UNIX AND NOT APPLE)
  target_sources(IgniteEngine PRIVATE core/device_manager_vulkan.cpp)

  target_compile_definitions(IgniteEngine PRIVATE PLATFORM_LINUX)
  target_include_directories(IgniteEngine /usr/include)
  target_link_directories(IgniteEngine PRIVATE /usr/lib)
  target_link_libraries(IgniteEngine PUBLIC nvrhi_vk)

  target_link_libraries(IgniteEngine PUBLIC
    vulkan shaderc_shared spriv-cross-core
    spirv-cross-glsl pthread dl m rt
  )

endif()

target_link_libraries(IgniteEngine PUBLIC nvrhi)

target_compile_definitions(IgniteEngine PRIVATE
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:RELEASE_BUILD>
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET IgniteEngine PROPERTY CXX_STANDARD 23)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/scripting)
