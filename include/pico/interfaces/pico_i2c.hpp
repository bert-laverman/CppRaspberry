#pragma once
// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the I2C bus on the Raspberry Pi Pico

#include <ios>
#include <vector>
#include <iostream>
#include <iomanip>

#include <pico/stdlib.h>
#include "hardware/i2c.h"

#include <interfaces/i2c.hpp>


namespace nl::rakis::raspberrypi::interfaces {

    class PicoI2C : public I2C
    {
        constexpr static const uint baudrate = 100000;

        i2c_inst_t *interface_{ nullptr };

        inline uint8_t readByteRaw() {
            return i2c_read_byte_raw(interface_);
        }
        inline void readBytesRaw(std::vector<uint8_t> &date) {
            i2c_read_raw_blocking(interface_, date.data(), date.size());
        }

    protected:
        virtual std::ostream &log() {
            return std::cout;
        }

    public:
        PicoI2C() = default;

        PicoI2C(i2c_inst_t *interface, unsigned sdaPin, unsigned sclPin);

        PicoI2C(unsigned sdaPin, unsigned sclPin) : PicoI2C(i2c0, sdaPin, sclPin)
        {
        }

        PicoI2C(const PicoI2C &) = default;
        PicoI2C(PicoI2C &&) = default;
        PicoI2C &operator=(const PicoI2C &) = default;
        PicoI2C &operator=(PicoI2C &&) = default;
        ~PicoI2C() = default;


        static PicoI2C &defaultInstance();

        virtual void open() override;
        virtual void close() override;

        virtual void switchToControllerMode() override;

        virtual void switchToResponderMode(uint8_t address, MsgCallback cb) override;

        virtual bool writeBytes(uint8_t address, std::span<uint8_t> data) override
        {
            auto result = i2c_write_blocking(interface_, address, data.data(), data.size(), false);
            if (result == PICO_ERROR_GENERIC) {
                log() << "Failed to write bytes to 0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(address) << ". No one there.\n";
                return false;
            } else if (result < 0) {
                log() << "Failed to write bytes to 0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(address) << ". Errno=" << std::dec << result << "\n";
                return false;
            } else if (static_cast<unsigned>(result) != data.size()) {
                log() << "Failed to write " << std::dec << data.size() << " bytes to 0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(address) << ". Only wrote " << std::dec << result << "\n";
                return false;
            }
            return true;
        }
    };
}