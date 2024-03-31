#pragma once
// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the I2C bus on the Raspberry Pi Pico

#include <vector>

#include <pico/stdlib.h>
#include "hardware/i2c.h"

#include <interfaces/i2c.hpp>


namespace nl::rakis::raspberrypi::interfaces {

    class PicoI2C : public virtual I2C
    {
        constexpr static const uint baudrate = 100000;

        i2c_inst_t *interface_;

        unsigned sdaPin_;
        unsigned sclPin_;

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
        PicoI2C(i2c_inst_t *interface, unsigned sdaPin, unsigned sclPin)
            : interface_(interface), sdaPin_(sdaPin), sclPin_(sclPin)
        {
            log() << "Initialising I2C on pins " << sdaPin << " and " << sclPin << std::endl;
            i2c_init(interface_, baudrate);

            gpio_set_function(sdaPin_, GPIO_FUNC_I2C);
            gpio_set_function(sclPin_, GPIO_FUNC_I2C);
            gpio_pull_up(sdaPin_);
            gpio_pull_up(sclPin_);
        }

        PicoI2C(unsigned sdaPin, unsigned sclPin) : PicoI2C(i2c0, sdaPin, sclPin)
        {
        }
        PicoI2C() : PicoI2C(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN)
        {
        }

        virtual void reset() override
        {
            i2c_init(interface_, baudrate);
        }

        virtual void switchToMaster() override
        {
            log() << "Switching to master mode\n";
            reset();
        }

        virtual void switchToSlave(uint8_t address, SlaveCallback cb) override;

        virtual void writeByte(uint8_t address, uint8_t value) override
        {
            i2c_write_blocking(interface_, address, &value, 1, false);
        }

        virtual void writeBytes(uint8_t address, std::span<uint8_t> data) override
        {
            i2c_write_blocking(interface_, address, data.data(), data.size(), false);
        }

        virtual void writeByteData(uint8_t address, uint8_t value, uint8_t data) override
        {
            uint8_t buffer[2] = {value, data};
            i2c_write_blocking(interface_, address, buffer, 2, false);
        }
    };
}