add_executable(IgniteEditor
  src/editor_layer.cpp
  src/editor_app.cpp
  src/panels/scene_panel.cpp
  src/render_target.cpp
)

target_include_directories(IgniteEditor PRIVATE
  src
  ${IGNITE_DIR}/src
  ${THIRDPARTY}/GLFW/src
  ${THIRDPARTY}/GLFW/include
  ${THIRDPARTY}/NVRHI/include
  ${THIRDPARTY}/BOX2D/include
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

target_link_libraries(IgniteEditor PRIVATE IgniteEngine)

if (WIN32)
  target_compile_definitions(IgniteEditor PRIVATE 
    PLATFORM_WINDOWS
    NOMINMAX
    WIN32_LEAN_AND_MEAN
  )

  if (IGNITE_WITH_DX12)
    target_compile_definitions(IgniteEditor PRIVATE IGNITE_WITH_DX12)
    message(">> Ignite Editor: Build with DX12")
  endif()
    
  if (IGNITE_WITH_VULKAN)

    message(">> Ignite Editor: Build with Vulkan")
    target_compile_definitions(IgniteEditor PRIVATE IGNITE_WITH_VULKAN)

    target_link_directories(IgniteEditor PRIVATE ${VULKAN_LIBRARY_DIR})

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_link_libraries(IgniteEditor PRIVATE
        vulkan-1.lib
        shaderc_sharedd.lib
        SPIRVd.lib
        spirv-cross-cored.lib
        spirv-cross-glsld.lib
        spirv-cross-hlsld.lib
        spirv-cross-reflectd.lib
        spirv-cross-utild.lib
        SPIRV-Toolsd.lib
      )
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
      target_link_libraries(IgniteEditor PRIVATE
        vulkan-1.lib
        shaderc_shared.lib
        SPIRV.lib
        spirv-cross-core.lib
        spirv-cross-glsl.lib
        spirv-cross-hlsl.lib
        spirv-cross-reflect.lib
        spirv-cross-util.lib
        SPIRV-Tools.lib
      )
    endif()
    
  endif()

elseif(UNIX AND NOT APPLE)

endif()

target_compile_definitions(IgniteEditor PRIVATE
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:RELEASE_BUILD>
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET IgniteEditor PROPERTY CXX_STANDARD 23)
endif()