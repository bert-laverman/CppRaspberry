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


message(STATUS "Adding Zero 2W specific i2c files")

set(CPP_RASPBERRY_INCLUDES ${CPP_RASPBERRY_INCLUDES}
    ${CMAKE_CURRENT_LIST_DIR}/include)

set(CPP_RASPBERRY_SOURCES ${CPP_RASPBERRY_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/src/interfaces/pigpiod-i2c.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/interfaces/pigpiod-bsc-i2c.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/interfaces/i2cdev-i2c.cpp)

set(CPP_RASPBERRY_LIBS ${CPP_RASPBERRY_LIBS} i2c pigpiod_if2)
