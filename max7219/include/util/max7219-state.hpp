#pragma once
// Copyright (c) 2024 by Bert Laverman, all rights reserved

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