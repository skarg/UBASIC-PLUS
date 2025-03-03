project(basic)
cmake_minimum_required(VERSION 3.20)

cmake_path(SET LIBRARY_SRC NORMALIZE "../../src")
cmake_path(SET LIBRARY_UBASIC NORMALIZE "${LIBRARY_SRC}/ubasic")
cmake_path(SET LIBRARY_POSIX NORMALIZE "../../ports/posix")

add_subdirectory(${LIBRARY_UBASIC} ubasic)
add_subdirectory(${LIBRARY_POSIX} posix)
add_executable(${PROJECT_NAME}
	main.c
)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
	${LIBRARY_SRC}
)
target_link_libraries(${PROJECT_NAME} ubasic posix)
