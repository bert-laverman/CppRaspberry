# Copyright (c) 2023-2024 by Bert Laverman. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# We require a target app name to be defined
if(NOT APP_NAME)
    message(FATAL_ERROR "APP_NAME is not defined")
endif(NOT APP_NAME)

set(CPP_RASPBERRY_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/include)
set(CPP_RASPBERRY_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/raspberry-pi.cpp)
set(CPP_RASPBERRY_LIBS "")

# The target platform is set with a "TARGET_XYZ" variable.
if(TARGET_PICO)
    include(${CMAKE_CURRENT_LIST_DIR}/platforms/pico/pico.cmake)
elseif(TARGET_ZERO2W)
    include(${CMAKE_CURRENT_LIST_DIR}/platforms/zero2w/zero2w.cmake)
endif(TARGET_PICO)

# Add in interface specific stuff

if(HAVE_I2C)
    include(${CMAKE_CURRENT_LIST_DIR}/interfaces/i2c/i2c.cmake)
endif(HAVE_I2C)

if(HAVE_SPI)
    include(${CMAKE_CURRENT_LIST_DIR}/interfaces/spi/spi.cmake)
endif(HAVE_SPI)

# And also add device specific stuff

if(HAVE_LCD2X16)
    include(${CMAKE_CURRENT_LIST_DIR}/devices/lcd2x16/lcd2x16.cmake)
endif(HAVE_LCD2X16)

if(HAVE_MAX7219)
    include(${CMAKE_CURRENT_LIST_DIR}/devices/max7219/max7219.cmake)
endif(HAVE_MAX7219)

if(HAVE_MCP23S17)
include(${CMAKE_CURRENT_LIST_DIR}/devices/mcp23s17/mcp23s17.cmake)
endif(HAVE_MCP23S17)

if(HAVE_TLC59711)
include(${CMAKE_CURRENT_LIST_DIR}/devices/tlc59711/tlc59711.cmake)
endif(HAVE_TLC59711)

# And components

if(HAVE_LED)
include(${CMAKE_CURRENT_LIST_DIR}/components/led/led.cmake)
endif(HAVE_LED)

if(HAVE_7SEGMENT)
include(${CMAKE_CURRENT_LIST_DIR}/components/7segment/7segment.cmake)
endif(HAVE_7SEGMENT)

if(HAVE_BUTTON)
include(${CMAKE_CURRENT_LIST_DIR}/components/button/button.cmake)
endif(HAVE_BUTTON)

# Finally, a macro to add all this to your local config.

macro(add_cpp_raspberry_app APP_NAME)
    message(STATUS "Adding CppRaspberry library to ${APP_NAME}")

    target_include_directories(${APP_NAME} PUBLIC ${CPP_RASPBERRY_INCLUDES})
    target_sources(${APP_NAME} PUBLIC ${CPP_RASPBERRY_SOURCES})
    target_link_libraries(${APP_NAME} ${CPP_RASPBERRY_LIBS})
endmacro(add_cpp_raspberry_app)