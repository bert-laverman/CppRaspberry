cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

if (TARGET_PICO)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/bertl/pico-sdk")

set(PICO_BOARD pico CACHE STRING "Board type")

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

endif (TARGET_PICO)

project(pico-test C CXX ASM)

if (TARGET_PICO)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

endif (TARGET_PICO)

add_executable(pico-test ${CMAKE_CURRENT_LIST_DIR}/src/raspberry_pi.cpp ${CMAKE_CURRENT_LIST_DIR}/src/pico-test.cpp)

if (TARGET_PICO)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(pico-test)

pico_set_program_name(pico-test "pico-test")
pico_set_program_version(pico-test "0.1")

pico_enable_stdio_uart(pico-test 1)
pico_enable_stdio_usb(pico-test 0)

add_compile_definitions(TARGET_PICO)

else

add_compile_definitions(TARGET_ZERO2W)

endif (TARGET_PICO)

target_include_directories(pico-test PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_compile_options(pico-test PRIVATE -Wall -Wextra -Werror)

if (TARGET_PICO)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(pico-test
    hardware_spi
    pico_stdlib)

endif (TARGET_PICO)