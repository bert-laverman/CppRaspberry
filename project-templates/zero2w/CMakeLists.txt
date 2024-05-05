# CMakeLists template for Raspberry Pi Pico projects
#

# Set your target app name here
set(APP_NAME replace-me)

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# Set CMake defines for the Raspberry Pi Pico and required components
set(TARGET_PICO off)
set(TARGET_ZERO2W on)
set(HAVE_I2C on)
set(HAVE_SPI off)
set(HAVE_PWM off)

#
# The CppRaspberry library location is taken from the CPP_RASPBERRY_PATH environment variable
# set(CPP_RASPBERRY_PATH $ENV{CPP_RASPBERRY_PATH})
# If this is not set, you can set it here
set(CPP_RASPBERRY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../CppRaspberry")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(${APP_NAME} C CXX ASM)
set(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-psabi -pthread -static")

# include the CppRaspberry library
include(${CPP_RASPBERRY_PATH}/CppRaspberry.cmake)

# And now our local project
#add_executable(${APP_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/test-i2c.cpp)

# Add CppRaspberry library
add_cpp_raspberry_app(${APP_NAME})

target_include_directories(${APP_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_compile_options(${APP_NAME} PRIVATE -Wall -Wextra -Werror)

target_link_libraries(${APP_NAME} pigpiod_if2)
