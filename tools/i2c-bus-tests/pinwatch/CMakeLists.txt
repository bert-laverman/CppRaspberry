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
project(pinwatch C CXX ASM)
pico_sdk_init()
add_executable(pinwatch pinwatch.c)
target_link_libraries(pinwatch pico_stdlib)
pico_enable_stdio_usb(pinwatch 1)
pico_enable_stdio_uart(pinwatch 0)
pico_add_extra_outputs(pinwatch)
