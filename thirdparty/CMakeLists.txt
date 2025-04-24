include(imgui.cmake)
include(stbi.cmake)
add_subdirectory(NVRHI)
add_subdirectory(SPDLOG)

add_subdirectory(BOX2D)
add_subdirectory(ShaderMake)

if (NOT TARGET glfw)
    option(GLFW_BUILD_EXAMPLES "" OFF)
    option(GLFW_BUILD_TESTS "" OFF)
    option(GLFW_BUILD_DOCS "" OFF)
    option(GLFW_INSTALL "" OFF)
    add_subdirectory(GLFW)
endif()

