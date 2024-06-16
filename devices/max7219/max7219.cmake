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


message(STATUS "Adding files for the SPI connected MAX7219 numeric display driver")

if(NOT HAVE_SPI)
message(FATAL_ERROR "A SPI interface is required for the MAX7219 driver")
endif()

add_compile_definitions(HAVE_MAX7219)

set(CPP_RASPBERRY_INCLUDES ${CPP_RASPBERRY_INCLUDES}
    ${CMAKE_CURRENT_LIST_DIR}/include)
