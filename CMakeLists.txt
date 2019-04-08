cmake_minimum_required(VERSION 3.4)
project(mselph264
    LANGUAGES CXX
    VERSION 0.4.0
)

find_program(CLANG_TIDY_BIN clang-tidy HINTS)
if(CLANG_TIDY_BIN)
    message(STATUS "Use clang-tidy")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

option(ENABLE_TEST "Enable test programs using the plugin" OFF)
option(ENABLE_TOOLS "Enable test tools for manual checking" OFF)
option(ENABLE_DOC "Enable generating documentation" OFF)

add_subdirectory(src)

if(ENABLE_TOOLS)
    add_subdirectory(tools)
endif()

if(ENABLE_DOC)
    add_subdirectory(docs)
endif()