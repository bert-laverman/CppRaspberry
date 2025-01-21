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
set(CPP_RASPBERRY_SOURCES "")
#set(CPP_RASPBERRY_SOURCES
#    ${CMAKE_CURRENT_LIST_DIR}/src/raspberry-pi.cpp)
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

file (STRINGS ${CMAKE_CURRENT_LIST_DIR}/devices/devices.txt ALL_DEVICES)
foreach (device ${ALL_DEVICES})
    if (NOT device MATCHES "^[ \t\r\n]*#.*")
        if(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/devices/${device})
            include(${CMAKE_CURRENT_LIST_DIR}/devices/${device}/${device}.cmake)
        endif()
    endif()
endforeach()

# And components

file (STRINGS ${CMAKE_CURRENT_LIST_DIR}/components/components.txt ALL_COMPONENTS)
foreach (component ${ALL_COMPONENTS})
    if (NOT component MATCHES "^[ \t\r\n]*#.*")
        if(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/components/${component})
            include(${CMAKE_CURRENT_LIST_DIR}/components/${component}/${component}.cmake)
        endif()
    endif()
endforeach()

# Finally, a macro to add all this to your local config.

macro(add_cpp_raspberry_app APP_NAME)
    message(STATUS "Adding CppRaspberry library to ${APP_NAME}")

    target_include_directories(${APP_NAME} PUBLIC ${CPP_RASPBERRY_INCLUDES})
    target_sources(${APP_NAME} PUBLIC ${CPP_RASPBERRY_SOURCES})
    target_link_libraries(${APP_NAME} ${CPP_RASPBERRY_LIBS})
endmacro(add_cpp_raspberry_app)