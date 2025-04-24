## -----------------------------
## GAME EXECUTABLE
add_executable(Game
    game/main.cpp
)

target_include_directories(Game PRIVATE
    ${IGNITE_DIR}/src
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
    ${VULKAN_INCLUDE_DIR}
)

target_link_libraries(Game PRIVATE
    IgniteEngine IgniteScriptCore
)

## -----------------------------
## GAME DLL

add_library(GameAssembly SHARED
    game_dll/export.cpp
    game_dll/game.cpp
)

target_include_directories(GameAssembly PRIVATE
    ${IGNITE_DIR}/src
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
    ${VULKAN_SDK}/Include
    ${VULKAN_INCLUDE_DIR}
)

target_link_libraries(GameAssembly PRIVATE
    IgniteEngine IgniteScriptCore
)

if (MSVC)
    target_compile_definitions(GameAssembly
        PRIVATE -DIGNITE_EXPORT_DLL
    )
endif()

