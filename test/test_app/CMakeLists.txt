add_executable(TestApp 
    src/app.cpp
    src/test_layer.cpp
)

target_include_directories(TestApp PRIVATE
    ${IGNITE_DIR}/src
    ${THIRDPARTY}/GLM
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
    endif()
    
  if (IGNITE_WITH_VULKAN)
    target_compile_definitions(TestApp PRIVATE IGNITE_WITH_VULKAN)
  endif()

elseif(UNIX AND NOT APPLE)

endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TestApp PROPERTY CXX_STANDARD 23)
endif()