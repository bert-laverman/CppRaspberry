# Copyright (c) 2026 by Bert Laverman. All Rights Reserved.
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

cmake_minimum_required(VERSION 3.13)
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
set(PICO_BOARD pico CACHE STRING "Board type")
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(collision C CXX ASM)
pico_sdk_init()
add_executable(collision collision.c)
target_link_libraries(collision pico_stdlib hardware_i2c)
pico_enable_stdio_usb(collision 1)
pico_enable_stdio_uart(collision 0)
pico_add_extra_outputs(collision)

set(DELAY_US 0 CACHE STRING "Delay after the trigger in microseconds")
target_compile_definitions(collision PRIVATE DELAY_US=${DELAY_US})
