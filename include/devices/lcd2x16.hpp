#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the simple 2-line, 16 character LCD display

#include <interfaces/i2c.hpp>

namespace nl::rakis::raspberrypi::devices {

class LCD2x16 {
    interfaces::I2C& i2c_;

    void write(std::string const& text) {
        i2c_.write(0x27, std::span<uint8_t>(reinterpret_cast<uint8_t const*>(text.data()), text.size()));
    }

public:
    LCD2x16(interfaces::I2C& i2c) : i2c_(i2c) {}
    LCD2x16(LCD2x16 const&) = default;
    LCD2x16(LCD2x16&&) = default;
    LCD2x16& operator=(LCD2x16 const&) = default;
    LCD2x16& operator=(LCD2x16&&) = default;
    virtual ~LCD2x16() = default;

    void clear() {
        write("\x1b[2J");
    }
};

} // namespace nl::rakis::raspberrypi::devices