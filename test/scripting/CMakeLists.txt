## -----------------------------
## GAME EXECUTABLE
add_executable(Game
    game/main.cpp
)

target_include_directories(Game PRIVATE
    ${IGNITE_DIR}/src
    ${THIRDPARTY}/GLM
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
    ${THIRDPARTY}/GLM
)

target_link_libraries(GameAssembly PRIVATE
    IgniteEngine IgniteScriptCore
)

if (MSVC)
    target_compile_definitions(GameAssembly
        PRIVATE -DIGNITE_EXPORT_DLL
    )
endif()

