/*
 * Copyright (c) 2023-2024 by Bert Laverman. All Rights Reserved.
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
    i2c_.write(address_, dataSpan);
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
    i2c_.write(address_, dataSpan);
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