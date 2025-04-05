# Required dependencies
set(third_party_folder "Third-Party Libraries")

add_subdirectory(NVRHI)
add_subdirectory(SPDLOG)

if (NOT TARGET ShaderMake)
    add_subdirectory(ShaderMake)
endif()

if (NOT TARGET glfw)
    option(GLFW_BUILD_EXAMPLES "" OFF)
    option(GLFW_BUILD_TESTS "" OFF)
    option(GLFW_BUILD_DOCS "" OFF)
    option(GLFW_INSTALL "" OFF)
    add_subdirectory(GLFW)
    set_target_properties(glfw PROPERTIES FOLDER ${third_party_folder})
    set_target_properties(update_mappings PROPERTIES FOLDER ${third_party_folder})
endif()

