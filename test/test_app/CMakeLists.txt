add_executable(TestApp 
    src/app.cpp
    src/test_layer.cpp
)

target_link_libraries(TestApp PRIVATE IgniteEngine)

target_include_directories(TestApp PRIVATE
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
    ${VULKAN_INCLUDE_DIR}
)

if (WIN32)
  target_compile_definitions(TestApp PRIVATE 
    PLATFORM_WINDOWS
    NOMINMAX
    WIN32_LEAN_AND_MEAN
  )

  if (IGNITE_WITH_DX12)
    target_compile_definitions(TestApp PRIVATE IGNITE_WITH_DX12)
    message(">> TestApp: Build with DX12")
  endif()
    
  if (IGNITE_WITH_VULKAN)
    message(">> TestApp: Build with Vulkan")
    target_compile_definitions(TestApp PRIVATE IGNITE_WITH_VULKAN)

    target_link_directories(TestApp PRIVATE ${VULKAN_LIBRARY_DIR})

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_link_libraries(TestApp PRIVATE
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
      target_link_libraries(TestApp PRIVATE
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

endif()

target_compile_definitions(TestApp PRIVATE
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:RELEASE_BUILD>
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TestApp PROPERTY CXX_STANDARD 23)
endif()