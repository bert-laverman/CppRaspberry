# Copyright (c) 2024 by Bert Laverman. All Rights Reserved.
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

if (HAVE_LCD2X16)

    message(STATUS "Adding files for the I2C connected 2x16 LCD display")

    if(NOT HAVE_I2C)
        message(FATAL_ERROR "An I2C interface is required for the LCD2x16 device")
    endif()

    set(CPP_RASPBERRY_INCLUDES ${CPP_RASPBERRY_INCLUDES}
        ${CMAKE_CURRENT_LIST_DIR}/include)

    set(CPP_RASPBERRY_SOURCES ${CPP_RASPBERRY_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/src/devices/lcd2x16.cpp)

endif()