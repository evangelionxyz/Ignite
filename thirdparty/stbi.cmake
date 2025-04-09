add_library(stblib
    ${CMAKE_CURRENT_SOURCE_DIR}/STB/stb_image.cpp
)

target_include_directories(stblib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/STB/include
)