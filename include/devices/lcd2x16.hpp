#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the simple 2-line, 16 character LCD display

#include <span>
#include <string>

#include <interfaces/i2c.hpp>

namespace nl::rakis::raspberrypi::devices {

class LCD2x16 {
    constexpr static const uint8_t LCD_BACKLIGHT = 0x08;

    interfaces::I2C& i2c_;
    unsigned address_{0x27};

    inline void writeCmd(uint8_t cmd) {
        i2c_.writeByte(address_, cmd);
    }

    inline void writeCmdData(uint8_t cmd, uint8_t data) {
        i2c_.writeByteData(address_, cmd, data);
    }

    inline void write4Bits(uint8_t value) {
        writeCmd(value | LCD_BACKLIGHT);
        writeCmd(value | 0x04 | LCD_BACKLIGHT);
        RaspberryPi::instance()->sleepMs(5);
        writeCmd((value & ~0x04) | LCD_BACKLIGHT);
        RaspberryPi::instance()->sleepMs(1);
    }

    inline void write8Bits(uint8_t value, bool charMode =false) {
        write4Bits((value & 0xf0) + (charMode ? 0x01 : 0x00));
        write4Bits(((value << 4) & 0xf0) + (charMode ? 0x01 : 0x00));
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

    inline void initDisplay() {
        write8Bits(0x03);
        write8Bits(0x03);
        write8Bits(0x03);
        write8Bits(0x02);
        write8Bits(0x20|0x08|0x00);
        write8Bits(0x08|0x04);
        write8Bits(0x01);
        write8Bits(0x04|0x02);
        RaspberryPi::instance()->sleepMs(200);
    }

    inline void clear() {
        write8Bits(0x01);
    }
    inline void home() {
        write8Bits(0x02);
    }
    inline void dark() {
        writeCmd(0x00);
    }
    inline void light() {
        writeCmd(0x08);
    }

    inline void print(unsigned line, const std::string s) {
        write8Bits(uint8_t(0x80 | (line ? 0x40 : 0x00)));
        for (auto c : s) {
            write8Bits(uint8_t(c | 0x01), true);
        }
    }
};

} // namespace nl::rakis::raspberrypi::devices