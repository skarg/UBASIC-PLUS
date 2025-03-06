# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# set the project name
project(uBASIC-Plus
    VERSION 1.0.0
    LANGUAGES C)

# set the C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Cross compilers and tools
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Build types
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_definitions(DEBUG)
    add_compile_options(-Og -g3)
endif ()

# Set the compiler options
if (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "AppleClang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
    # enable all relevant warnings that find bugs
    add_compile_options(-Wall -Wextra -pedantic)
    add_compile_options(-Wfloat-equal -Wconversion)
    add_compile_options(-Wfloat-conversion -Wdouble-promotion)
    add_compile_options(-Wredundant-decls -Wmissing-declarations)
    add_compile_options(-Wswitch-default)
    add_compile_options(-Wunused-variable)
    add_compile_options(-Wcast-qual)
    add_compile_options(-Wwrite-strings)
    # don't warn about conversion, sign, compares, long long and attributes
    add_compile_options(-Wno-sign-conversion -Wno-conversion)
    add_compile_options(-Wno-sign-compare)
    add_compile_options(-Wno-implicit-fallthrough)
    if (APPLE)
        # Apple defines UINT64_MAX with ULL suffix. This is not C90 compliant.
        add_compile_options(-Wno-long-long)
    endif()
endif()

message(STATUS "Binary Directory: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "Source Directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_C_FLAGS_DEBUG: ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")

add_subdirectory(apps/basic)

find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
  file(GLOB_RECURSE PRETTY_SRC "src/*.[c,h]")
  add_custom_target(
    pretty
    COMMAND ${CLANG_FORMAT} -i -style=file -fallback-style=none ${PRETTY_SRC}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Formatting code with clang-format"
  )
endif()