#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: MAX7219 LED driver

#include <array>
#include <vector>

#include <interfaces/spi.hpp>


namespace nl::rakis::raspberrypi::devices {

class MAX7219 {
public:
    constexpr static uint8_t CMD_NOOP = 0x00;
    constexpr static uint8_t CMD_DIGIT0 = 0x01;
    constexpr static uint8_t CMD_DIGIT1 = 0x02;
    constexpr static uint8_t CMD_DIGIT2 = 0x03;
    constexpr static uint8_t CMD_DIGIT3 = 0x04;
    constexpr static uint8_t CMD_DIGIT4 = 0x05;
    constexpr static uint8_t CMD_DIGIT5 = 0x06;
    constexpr static uint8_t CMD_DIGIT6 = 0x07;
    constexpr static uint8_t CMD_DIGIT7 = 0x08;
    constexpr static uint8_t CMD_DECODEMODE = 0x09;
    constexpr static uint8_t CMD_BRIGHTNESS = 0x0A;
    constexpr static uint8_t CMD_SCANLIMIT = 0x0B;
    constexpr static uint8_t CMD_SHUTDOWN = 0x0C;
    constexpr static uint8_t CMD_DISPLAYTEST = 0x0F;

private:
    interfaces::SPI& spi_;

    std::vector<std::array<uint8_t, 8>> buffer_;
    bool padding_{false};
    bool writeImmediately_{true};

    bool verbose_{false};

public:
    MAX7219(interfaces::SPI& spi) : spi_(spi) {
        buffer_.resize(spi_.numModules());
        for (auto &module : buffer_) {
            module.fill(0);
        }
    }

    void setBrightness(uint8_t value) {
        std::array<uint8_t, 2> buf{CMD_BRIGHTNESS, value};
        spi_.writeAll(buf);
    }

    void setScanLimit(uint8_t value) {
        std::array<uint8_t, 2> buf{CMD_SCANLIMIT, value};
        spi_.writeAll(buf);
    }

    void setDecodeMode(uint8_t value) {
        std::array<uint8_t, 2> buf{CMD_DECODEMODE, value};
        spi_.writeAll(buf);
    }

    void shutdown() {
        std::array<uint8_t, 2> buf{CMD_SHUTDOWN, 0};
        spi_.writeAll(buf);
    }

    void startup() {
        std::array<uint8_t, 2> buf{CMD_SHUTDOWN, 1};
        spi_.writeAll(buf);
    }

    void displayTest(uint8_t value) {
        std::array<uint8_t, 2> buf{CMD_DISPLAYTEST, value};
        spi_.writeAll(buf);
    }

    inline void sendData() const {
        for (unsigned pos = 0; pos < 8; ++pos) {
            spi_.writeAll([pos,this](unsigned module){ return std::array<uint8_t, 2>{uint8_t(CMD_DIGIT0 + pos), buffer_[module][pos]}; });
        }
    }

    inline bool writeImmediately() const { return writeImmediately_; }
    inline void writeImmediately(bool value) { writeImmediately_ = value; }

    inline bool verbose() const { return verbose_; }
    inline void verbose(bool value) { verbose_ = value; }
    
    inline void clear(unsigned module) {
        buffer_[module].fill(0x0f);
        if (writeImmediately()) sendData();
    }

    inline void setNumber(unsigned module, unsigned value) {
        buffer_[module].fill(0);

        for (unsigned pos = 0; (pos < 8); ++pos, value /= 10) {
            if ((value > 0) || padding_ || (pos == 0)) {
                buffer_[module][pos] = (value % 10);
            } else {
                buffer_[module][pos] = 0x0f;
            }
        }
        if (writeImmediately()) sendData();
    }
};

} // namespace nl::rakis::raspberrypi::devices