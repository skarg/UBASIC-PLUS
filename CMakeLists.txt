# This is a CMake build recipe using the ARM GCC compiler
#
# To build this project you need to install:
# - ARM GCC compiler
# - CMake
#
# To build this project you need to run:
# - cmake -S . -B build
# - cmake --build build
#
# You can also use VSCode with Cortex-Debug extension
#
cmake_minimum_required(VERSION 3.20)

# Cross compilers and tools
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)
set(CMAKE_NM arm-none-eabi-nm)
set(CMAKE_GDB gdb-multiarch)
set(CMAKE_OPENOCD openocd)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(EXECUTABLE ${PROJECT_NAME}.elf)

project(ubasic-plus)

enable_language(C ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Specific ARM microcontroller compiler and linker settings
add_compile_options(-mcpu=cortex-m4)
add_compile_options(-mthumb -mthumb-interwork)
add_compile_options(-ffunction-sections -fdata-sections)
add_compile_options(-fno-common -fmessage-length=0)
add_compile_options(-fstack-usage -fdump-rtl-dfinish)
add_link_options(-mcpu=cortex-m4)
add_link_options(-mthumb -mthumb-interwork)
add_link_options(-Wl,-gc-sections,--print-memory-usage)

# Enable hardware FPU
add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
add_compile_definitions(ARM_MATH_CM4;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)

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

set(LIBRARY_UBASIC_CORE "${CMAKE_SOURCE_DIR}/uBasic-Plus/core")

set(LIBRARY_UBASIC_SRC
    ${LIBRARY_UBASIC_CORE}/cli.c
    ${LIBRARY_UBASIC_CORE}/tokenizer.c
    ${LIBRARY_UBASIC_CORE}/ubasic.c
)

set(EXECUTABLE ${PROJECT_NAME}.elf)

add_executable(${EXECUTABLE} ${LIBRARY_UBASIC_SRC})

target_compile_definitions(${EXECUTABLE} PRIVATE
    -DNDEBUG
)

# inhibit pedantic warnings
target_compile_options(${EXECUTABLE} PRIVATE
    -Wall -Wextra -pedantic
    -Wfloat-equal -Wconversion -Wredundant-decls
    # don't warn about conversion, sign, compares, long long and attributes
    # since they are common in embedded
    #-Wno-sign-conversion
    -Wno-conversion
    #-Wno-sign-compare
    #-Wno-long-long
    #-Wno-attributes
    # don't warn about implicit fallthrough since it is common in network protocols
    -Wno-switch-default
    -Wno-implicit-fallthrough
    # the SDK does not meet coding guidelines
    #-Wno-comment
    #-Wno-missing-braces
    #-Wno-unused-variable
    # reference the linker file for CSTACK size
    -Wstack-usage=16384
    #-Wno-unused-parameter
    -Wno-char-subscripts
)

target_include_directories(${EXECUTABLE} PRIVATE
    ${LIBRARY_UBASIC_CORE}
)

target_link_options(${EXECUTABLE} PRIVATE
    -specs=nano.specs
    -lm
    -lnosys
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections
)

# Create hex and bin files
add_custom_command(TARGET ${EXECUTABLE}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${EXECUTABLE} ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${EXECUTABLE} ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${EXECUTABLE}  ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.hex
)

# Print executable size
add_custom_command(TARGET ${EXECUTABLE}
    POST_BUILD
    COMMAND ${CMAKE_SIZE} ${EXECUTABLE}
)
