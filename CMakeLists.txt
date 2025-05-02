cmake_minimum_required(VERSION 3.20)
project(Ignite)

set(IGNITE_WITH_VULKAN ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

option(SHADERMAKE_BUILD_TEST "Disabling ShaderMake Test program" OFF)

message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")

if (WIN32)
    set(IGNITE_WITH_DX12 ON) # enable DX12 by default for Windows

    message(">> Platform: WIN32")
    if (IGNITE_WITH_VULKAN)
    if (NOT DEFINED ENV{VULKAN_SDK})
        message(fatal_error ">> VULKAN_SDK environment variable not defined. Please install Vulkan SDK and set the environment variable.")
    endif()

    set(VULKAN_INCLUDE_DIR $ENV{VULKAN_SDK}\\Include)
    set(VULKAN_LIBRARY_DIR $ENV{VULKAN_SDK}\\Lib)

    message(">> Vulkan include dir: ${VULKAN_INCLUDE_DIR}")
    message(">> Vulkan lib dir: ${VULKAN_LIBRARY_DIR}")

    endif() #IGNITE_WITH_VULKAN

elseif(UNIX AND NOT APPLE)
    execute_process(COMMAND uname -r OUTPUT_VARIABLE WSL_KERNEL_NAME)
    string(FIND "${WSL_KERNEL_NAME}" "Microsoft" WSL_FOUND)

    if (WSL_FOUND EQUAL -1)
        message(">> Platform: WSL")
    else()
        message(">> Platform: LINUX")
    endif()
endif()

if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()

# Enable UTF-8 source encoding for all major compilers
if (MSVC)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

    # Optional debug info format tweak
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>"
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # GCC and Clang support -finput-charset=utf-8
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin-cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/MinSizeRel")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ReleaseDebInfo")

set(THIRDPARTY ${CMAKE_SOURCE_DIR}/thirdparty)

set(IGNITE_DIR ${CMAKE_SOURCE_DIR}/engine/ignite)
set(SCRIPTING_DIR ${CMAKE_SOURCE_DIR}/engine/scripting)

add_subdirectory(thirdparty)
add_subdirectory(engine/ignite)
add_subdirectory(engine/scripting)
add_subdirectory(editor)
add_subdirectory(test/test_app)
#add_subdirectory(test/scripting)
