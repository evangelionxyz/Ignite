add_library(IgniteScriptCore SHARED
    iscript.cpp
    script_component.cpp
)

if (MSVC)
    target_compile_definitions(IgniteScriptCore
        PRIVATE -DIGNITE_EXPORT_DLL
    )
endif()

target_include_directories(IgniteScriptCore PRIVATE
    ${IGNITE_DIR}/src/
    ${THIRDPARTY}/SPDLOG/include/
    ${THIRDPARTY}/GLM/
)

target_link_libraries(IgniteScriptCore PRIVATE
    IgniteEngine
)
