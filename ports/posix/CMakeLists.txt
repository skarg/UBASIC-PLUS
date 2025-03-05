cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(posix)

add_library(${PROJECT_NAME}
    ubasic-posix.c
)

cmake_path(SET LIBRARY_SRC NORMALIZE "../../src")
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBRARY_SRC}
)
