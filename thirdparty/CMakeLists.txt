include(imgui.cmake)
include(stbi.cmake)
add_subdirectory(NVRHI)
add_subdirectory(SPDLOG)

add_subdirectory(BOX2D)
add_subdirectory(ShaderMake)

if (NOT TARGET glfw)
    add_subdirectory(GLFW)
    
    option(GLFW_BUILD_EXAMPLES "" OFF)
    option(GLFW_BUILD_TESTS "" OFF)
    option(GLFW_BUILD_DOCS "" OFF)
    option(GLFW_INSTALL "" OFF)

    option(GLFW_BUILD_X11 "Build X11 support" ON)
    option(GLFW_BUILD_WAYLAND "Build Wayland support" OFF)

    if (UNIX AND NOT APPLE)
        target_compile_definitions(glfw PRIVATE GLFW_USE_X11)
    endif()
endif()

