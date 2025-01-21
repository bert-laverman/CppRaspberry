#pragma once
/*
 * Copyright (c) 2025 by Bert Laverman. All Rights Reserved.
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


#if !defined(HAVE_SPI)
#error "SPI support is required for the MAX7219 driver"
#endif

#include <array>
#include <interfaces/spi.hpp>
#include <devices/spi-device.hpp>

namespace nl::rakis::raspberrypi::devices {


template<unsigned Width =128, unsigned Height =32>
class SSD1305 : public SPIDevice {
    unsigned reset_{ 19 };  // Reset pin (Pin 27, GPIO 21)
    unsigned dc_{ 18 };     // Command/data pin (Pin 26, GPIO 20)

    static constexpr unsigned width_{ Width };
    static constexpr unsigned height_{ Height };
    std::array<uint8_t, (width_*height_)/8> buffer_;

    bool dirty_{ false };
    bool sendImediately_{ false };

    static constexpr unsigned pageSize{ 128 };

protected:
    virtual void numDevicesChanged() override { }

    void command(uint8_t cmd) {
        auto& gpio(RaspberryPi::instance().gpio());

        gpio.set(dc_, false);

        interface()->write(std::span<uint8_t>{std::addressof(cmd), 1});
    }

    void data(const std::span<uint8_t> buffer) {
        auto& gpio(RaspberryPi::instance().gpio());

        gpio.set(dc_, true);
        interface()->write(buffer);
    }

    void dirty() noexcept { dirty_ = true; }
    void clean() noexcept { dirty_ = false; }
    bool isDirty() const noexcept { return dirty_; }

public:
    SSD1305() = default;
    ~SSD1305() = default;

    SSD1305(const SSD1305&) = default;
    SSD1305(SSD1305&&) = default;
    SSD1305& operator=(const SSD1305&) = default;
    SSD1305& operator=(SSD1305&&) = default;

    unsigned dcPin() const noexcept { return dc_; }
    void dcPin(unsigned dc) noexcept { dc_ = dc; }

    unsigned rstPin() const noexcept { return reset_; }
    void rstPin(unsigned rst) noexcept { reset_ = rst; }

    unsigned width() const noexcept { return width_; }
    unsigned height() const noexcept { return height_; }

    void sendImmediately(bool doIt =true) noexcept { sendImediately_ = doIt; }
    bool isSendImmediately() const noexcept { return sendImediately_; }

    void reset() {
        auto& gpio(RaspberryPi::instance().gpio());

        if (gpio.available(reset_)) { gpio.init(reset_); }
        if (gpio.available(dc_)) { gpio.init(dc_); }

        gpio.set(reset_, true);
        RaspberryPi::instance().sleepMs(100);
        gpio.set(reset_, false);
        RaspberryPi::instance().sleepMs(100);
        gpio.set(interface()->csPin(), true);
        gpio.set(dc_, false);
        gpio.set(reset_, true);
        RaspberryPi::instance().sleepMs(100);

        command(0xAE);//--turn off oled panel
        command(0x04);//--Set Lower Column Start Address for Page Addressing Mode	
        command(0x10);//--Set Higher Column Start Address for Page Addressing Mode
        command(0x40);//---Set Display Start Line
        command(0x81);//---Set Contrast Control for BANK0
        command(0x80);//--Contrast control register is set
        command(0xA1);//--Set Segment Re-map
        command(0xA6);//--Set Normal/Inverse Display
        command(0xA8);//--Set Multiplex Ratio
        command(0x1F);//Set COM/Row Scan Direction   
        // command(0xC8);//--Set COM Output Scan Direction
        command(0xC0);//--Set COM Output Scan Direction
        command(0xD3);//--Set Display Offset
        command(0x00);//--1/64 duty
        command(0xD5);//--Set Display Clock Divide Ratio/ Oscillator Frequency
        command(0xF0);//-not offset
        command(0xD8);//--Set Area Color Mode ON/OFF & Low Power Display Mode
        command(0x05);//--set divide ratio, Set Clock as 100 Frames/Sec
        command(0xD9);//--Set pre-charge period
        command(0xC2);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
        command(0xDA);//--Set COM Pins Hardware Configuration
        command(0x12);
        command(0xDB);//--Set VCOMH Deselect Level
        command(0x08);//--Set VCOM Deselect Level
        command(0xAF);//--Normal Brightness Display ON
        RaspberryPi::instance().sleepMs(200);

        command(0xaf);
    }

    void sendBuffer() {
        if (isDirty()) {
            // printf("Sending pages ");
            const uint8_t nPages = buffer_.size() / pageSize;
            unsigned pos{ 0 };
            for (uint8_t page = 0; page < nPages; page++, pos += pageSize) {
                // printf("%d (pos=%d)... ", page, pos);
                /* set page address */
                command(0xB0 + page);
                /* set low column address */
                command(0x04); 
                /* set high column address */
                command(0x10); 
                /* write data */
                data(std::span<uint8_t>(&buffer_[pos], &buffer_[pos+pageSize]));
            }
            // printf("Done\n");
            clean();
        }
    }

    void clear() {
        buffer_.fill(0);
        if (isSendImmediately()) { sendBuffer(); } else { dirty(); }
    }

    void set(unsigned x, unsigned y, unsigned color) {
        if ((x < width_) && (y < height_)) {
            if (color) {
                buffer_[x + ((y >> 3) * width_)] |= 0b0000'0001 << (y & 0x07);
            }
            else {
                buffer_[x + ((y >> 3) * width_)] &= ~(0b0000'0001 << (y & 0x07));
            }
        }
        if (isSendImmediately()) { sendBuffer(); } else { dirty(); }
    }

    void set(unsigned x, unsigned y) {
        if ((x < width_) && (y < height_)) {
            buffer_[x + ((y >> 3) * width_)] |= 0b0000'0001 << (y & 0x07);
        }
        if (isSendImmediately()) { sendBuffer(); } else { dirty(); }
    }

    void reset(unsigned x, unsigned y) {
        if ((x < width_) && (y < height_)) {
            buffer_[x + ((y >> 3) * width_)] &= ~(0b0000'0001 << (y & 0x07));
        }
        if (isSendImmediately()) { sendBuffer(); } else { dirty(); }
    }

};

} // nl::rakis::raspberrypi::devices