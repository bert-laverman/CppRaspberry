#pragma once
/*
 * Copyright (c) 2024 by Bert Laverman. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <map>
#include <string>

#include <devices/max7219.hpp>


namespace nl::rakis::raspberrypi::devices {


constexpr static const char* MAX7219_FIELD_BRIGHTNESS = "brightness";
constexpr static const char* MAX7219_FIELD_SCANLIMIT = "scanlimit";
constexpr static const char* MAX7219_FIELD_DECODEMODE = "decodemode";
constexpr static const char* MAX7219_FIELD_VALUE = "value";


template <typename Max7219_type>
void loadState(const std::map<std::string, std::string>& state, Max7219_type& device, uint8_t module)
{
    const auto& brightness = state.find(MAX7219_FIELD_BRIGHTNESS);
    if (brightness != state.end()) {
        device.setBrightness(module, std::stoi(brightness->second));
    }
    const auto& scanlimit = state.find(MAX7219_FIELD_SCANLIMIT);
    if (scanlimit != state.end()) {
        device.setScanLimit(module, std::stoi(scanlimit->second));
    }
    const auto& decodemode = state.find(MAX7219_FIELD_DECODEMODE);
    if (decodemode != state.end()) {
        device.setDecodeMode(module, std::stoi(decodemode->second));
    }
    const auto& value = state.find(MAX7219_FIELD_VALUE);
    if (value != state.end()) {
        device.setNumber(module, std::stoi(value->second));
    } else {
        device.clear(module);
    }
}


template <typename Max7219_type>
void saveState(std::map<std::string, std::string>& state, const Max7219_type& device, uint8_t module)
{
    state[MAX7219_FIELD_BRIGHTNESS] = std::to_string(device.getBrightness(module));
    state[MAX7219_FIELD_SCANLIMIT] = std::to_string(device.getScanLimit(module));
    state[MAX7219_FIELD_DECODEMODE] = std::to_string(device.getDecodeMode(module));
    if (device.hasValue(module)) {
        state[MAX7219_FIELD_VALUE] = std::to_string(device.getValue(module));
    } else {
        state.erase(MAX7219_FIELD_VALUE);
    }
}

} // namespace nl::rakis::raspberrypi::devices