// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47

#include <array>

#include <devices/lcd2x16.hpp>

using nl::rakis::raspberrypi::RaspberryPi;
using nl::rakis::raspberrypi::devices::LCD2x16;

void LCD2x16::sendData(uint8_t data) {
    const uint8_t hiNibble = data & 0xf0;
    const uint8_t loNibble = (data << 4) & 0xf0;
    std::array<uint8_t, 4> dataSpan{
        static_cast<uint8_t>(hiNibble | (LCD_DATA_ON & backlightMask())),
        static_cast<uint8_t>(hiNibble | (LCD_DATA_OFF & backlightMask())),
        static_cast<uint8_t>(loNibble | (LCD_DATA_ON & backlightMask())),
        static_cast<uint8_t>(loNibble | (LCD_DATA_OFF & backlightMask()))
    };
    i2c_.writeBytes(address_, dataSpan);
}

void LCD2x16::sendCmd(uint8_t data) {
    const uint8_t hiNibble = data & 0xf0;
    const uint8_t loNibble = (data << 4) & 0xf0;
    std::array<uint8_t, 4> dataSpan{
        static_cast<uint8_t>(hiNibble | (LCD_CMD_ON & backlightMask())),
        static_cast<uint8_t>(hiNibble | (LCD_CMD_OFF & backlightMask())),
        static_cast<uint8_t>(loNibble | (LCD_CMD_ON & backlightMask())),
        static_cast<uint8_t>(loNibble | (LCD_CMD_OFF & backlightMask()))
    };
    i2c_.writeBytes(address_, dataSpan);
}

void LCD2x16::initDisplay()
 {
    sleep_for(15);
    sendCmd(LCD_HOME);
    sleep_for(5);
    sendCmd(LCD_HOME);
    sleep_for(5);
    sendCmd(LCD_HOME);
    sleep_for(5);
    sendCmd(LCD_FUNCTION_SET | LCD_FUNCTION_4BIT | LCD_FUNCTION_2LINE);
    sendCmd(LCD_DISPLAY_MODE | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF);
    sendCmd(LCD_CLEAR);
    sendCmd(LCD_ENTRY_MODE | LCD_ENTRY_INC);

    sleep_for(5);
    sendCmd(LCD_CRS_HOME_L1);
}