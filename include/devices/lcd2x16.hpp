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

    inline void byte(uint8_t value, uint8_t flags =0) {
        uint8_t cmd[] = { uint8_t((value & 0xf0) | flags), uint8_t(((value << 4) & 0xf0) | flags) };
        i2c_.write(address_, cmd, 1);
    }

public:
    LCD2x16(interfaces::I2C& i2c) : i2c_(i2c) {}
    LCD2x16(LCD2x16 const&) = default;
    LCD2x16(LCD2x16&&) = default;
    LCD2x16& operator=(LCD2x16 const&) = default;
    LCD2x16& operator=(LCD2x16&&) = default;
    virtual ~LCD2x16() = default;

    inline void setAddress(unsigned address) {
        address_ = address;
    }

    inline void clear() {
        byte(address_, 0x01);
    }
    inline void home() {
        byte(address_, 0x02);
    }
    inline void dark() {
        byte(address_, 0x00);
    }
    inline void light() {
        byte(address_, 0x0c);
    }

    inline void print(unsigned line, std::string s) {
        byte(uint8_t(0x80 | (line ? 0x40 : 0x00)));
        for (auto c : s) {
            byte(c, 0x01);
        }
    }
};

} // namespace nl::rakis::raspberrypi::devices