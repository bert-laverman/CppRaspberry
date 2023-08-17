#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the simple 2-line, 16 character LCD display

#include <span>
#include <string>

#include <interfaces/i2c.hpp>

namespace nl::rakis::raspberrypi::devices {

class LCD2x16 {
    interfaces::I2C& i2c_;
    unsigned address_{0x27};

public:
    LCD2x16(interfaces::I2C& i2c) : i2c_(i2c) {}
    LCD2x16(LCD2x16 const&) = default;
    LCD2x16(LCD2x16&&) = default;
    LCD2x16& operator=(LCD2x16 const&) = default;
    LCD2x16& operator=(LCD2x16&&) = default;
    virtual ~LCD2x16() = default;

    inline void clear() {
        uint8_t cmd[] = { 0x01 };
        i2c_.write(address_, cmd, 1);
    }
    inline void home() {
        uint8_t cmd[] = { 0x02 };
        i2c_.write(address_, cmd, 1);
    }
    inline void dark() {
        uint8_t cmd[] = { 0x00 };
        i2c_.write(address_, cmd, 1);
    }
    inline void light() {
        uint8_t cmd[] = { 0x0c };
        i2c_.write(address_, cmd, 1);
    }
    inline void print(unsigned line, std::string s) {
        uint8_t cmd[] = { uint8_t(0x80 | (line ? 0x40 : 0x00)) };
        i2c_.write(address_, cmd, 1);
        i2c_.write(address_, reinterpret_cast<uint8_t const*>(s.data()), s.size());
    }
};

} // namespace nl::rakis::raspberrypi::devices