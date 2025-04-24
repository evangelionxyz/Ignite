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
    endif()
    
  if (IGNITE_WITH_VULKAN)
    target_compile_definitions(IgniteEditor PRIVATE IGNITE_WITH_VULKAN)
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