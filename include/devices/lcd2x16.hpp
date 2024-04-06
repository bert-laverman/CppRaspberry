#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the simple 2-line, 16 character LCD display

#if !defined(HAVE_I2C)
#error "No I2C interface defined"
#endif

#include <span>
#include <string>

#if defined(TARGET_PICO)

#include <pico/stdlib.h>

static inline void sleep_for(unsigned ms) {
    sleep_ms(ms);
}

#else
#include <chrono>
#include <thread>

static inline void sleep_for(unsigned ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

#endif

#include <raspberry_pi.hpp>

#include <interfaces/i2c.hpp>

namespace nl::rakis::raspberrypi::devices {

class LCD2x16 {
    constexpr static const uint8_t LCD_DEFAULT_ADDRESS = 0x27;

    constexpr static const uint8_t LCD_CMD_ON = 0x0c;
    constexpr static const uint8_t LCD_CMD_OFF = 0x08;
    constexpr static const uint8_t LCD_DATA_ON = 0x0d;
    constexpr static const uint8_t LCD_DATA_OFF = 0x09;

    constexpr static const uint8_t LCD_BACKLIGHT = 0x08;
    constexpr static const uint8_t LCD_BACKLIGHT_ON = 0x0f;
    constexpr static const uint8_t LCD_BACKLIGHT_OFF = 0x07;

    // Clear display (high 7 bits 0)
    constexpr static const uint8_t LCD_CLEAR            = 0b00000001;

    // Home cursor (high 6 bits 0, bit 0 ignored)
    constexpr static const uint8_t LCD_HOME             = 0b00000010;

    // Entry mode command bits (high 5 bits 0)
    constexpr static const uint8_t LCD_ENTRY_MODE       = 0b00000100;
    constexpr static const uint8_t LCD_ENTRY_INC        = 0b00000010;
    constexpr static const uint8_t LCD_ENTRY_DEC        = 0b00000000;
    constexpr static const uint8_t LCD_ENTRY_SHIFT      = 0b00000001;

    // Display on/of command bits (high 4 bits 0)
    constexpr static const uint8_t LCD_DISPLAY_MODE     = 0b00001000;
    constexpr static const uint8_t LCD_DISPLAY_ON       = 0b00000100;
    constexpr static const uint8_t LCD_DISPLAY_OFF      = 0b00000000;
    constexpr static const uint8_t LCD_CURSOR_ON        = 0b00000010;
    constexpr static const uint8_t LCD_CURSOR_OFF       = 0b00000000;
    constexpr static const uint8_t LCD_BLINK_ON         = 0b00000001;
    constexpr static const uint8_t LCD_BLINK_OFF        = 0b00000000;

    // Cursor/display shift command bits (high 3 bits 0)
    constexpr static const uint8_t LCD_SHIFT            = 0b00010000;
    constexpr static const uint8_t LCD_SHIFT_DISPLAY    = 0b00001000;
    constexpr static const uint8_t LCD_SHIFT_CURSOR     = 0b00000000;
    constexpr static const uint8_t LCD_SHIFT_RIGHT      = 0b00000100;
    constexpr static const uint8_t LCD_SHIFT_LEFT       = 0b00000000;

    // Function set command bits (high 2 bits 0)
    constexpr static const uint8_t LCD_FUNCTION_SET     = 0b00100000;
    constexpr static const uint8_t LCD_FUNCTION_8BIT    = 0b00010000;
    constexpr static const uint8_t LCD_FUNCTION_4BIT    = 0b00000000;
    constexpr static const uint8_t LCD_FUNCTION_2LINE   = 0b00001000;
    constexpr static const uint8_t LCD_FUNCTION_1LINE   = 0b00000000;
    constexpr static const uint8_t LCD_FUNCTION_10DOTS  = 0b00000100;
    constexpr static const uint8_t LCD_FUNCTION_5X8DOTS = 0b00000000;

    // Set CGRAM address (high 1 bit 0)
    constexpr static const uint8_t LCD_CGRAM_SET        = 0b01000000;

    // Set DDRAM address (high bit 1)
    constexpr static const uint8_t LCD_DDRAM_SET        = 0b10000000;
    constexpr static const uint8_t LCD_CRS_HOME_L1      = (LCD_DDRAM_SET | 0x00);
    constexpr static const uint8_t LCD_CRS_HOME_L2      = (LCD_DDRAM_SET | 0x40);



    interfaces::I2C& i2c_;
    unsigned address_{ LCD_DEFAULT_ADDRESS };
    uint8_t backlightMask_{ LCD_BACKLIGHT_ON };

    inline uint8_t backlightMask() const {
        return backlightMask_;
    }

    inline void writeCmd(uint8_t cmd) {
        std::array<uint8_t, 1> data{cmd};
        i2c_.writeBytes(address_, data);
    }

    inline void write4Bits(uint8_t value) {
        writeCmd(value | LCD_BACKLIGHT);
        writeCmd(value | 0x04 | LCD_BACKLIGHT);
        sleep_for(5);
        writeCmd((value & ~0x04) | LCD_BACKLIGHT);
        sleep_for(1);
    }

    inline void write8Bits(uint8_t value, bool charMode =false) {
        write4Bits((value & 0xf0) + (charMode ? 0x01 : 0x00));
        write4Bits(((value << 4) & 0xf0) + (charMode ? 0x01 : 0x00));
    }

    void sendData(uint8_t data);
    void sendCmd(uint8_t cmd);

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

    void initDisplay();

    inline void clear() {
        sendCmd(LCD_CLEAR);
        sleep_for(5);
    }
    inline void home() {
        sendCmd(LCD_HOME);
        sleep_for(5);
    }
    inline void dark() {
        sendCmd(LCD_DISPLAY_MODE | LCD_DISPLAY_OFF | LCD_CURSOR_OFF | LCD_BLINK_OFF);
        sleep_for(5);
    }
    inline void light() {
        sendCmd(LCD_DISPLAY_MODE | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF);
        sleep_for(5);
    }

    inline void print(unsigned line, const std::string s) {
        sendCmd(line == 0 ? LCD_CRS_HOME_L1 : LCD_CRS_HOME_L2);
        for (auto c : s) {
            sendData(uint8_t(c));
        }
    }
};

} // namespace nl::rakis::raspberrypi::devices