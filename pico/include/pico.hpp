#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: General Pico management code

#include <iostream>

#include <pico/stdlib.h>
#include <pico/stdio.h>

#include <raspberry_pi.hpp>

#if defined(HAVE_I2C)
#include <hardware/i2c.h>

#include <interfaces/pico_i2c.hpp>
#endif

#if defined(HAVE_SPI)
#include <hardware/spi.h>

#include <interfaces/pico_spi.hpp>
#endif

namespace nl::rakis::raspberrypi {

class PICO
  : public virtual RaspberryPi<std::ostream
#if defined(HAVE_I2C)
    , interfaces::PicoI2C
#endif
#if defined(HAVE_SPI)
    , interfaces::PicoSPI
#endif
  >
{

public:
    PICO(bool verbose =false) : RaspberryPi(verbose)
    {
    };

    virtual ~PICO() =default;

    static PICO& instance(bool verbose =false) {
        static PICO instance{ verbose };
        return instance;
    }

#if defined(HAVE_I2C)

    /**
     * @brief Check if an I2C interface is available at the given pins.
     */
    inline bool haveI2C(unsigned sdaPin, unsigned sclPin) {
        for (auto it = i2cInterfaces().begin(); it != i2cInterfaces().end(); ++it) {
            if (it->sdaPin() == sdaPin && it->sclPin() == sclPin) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Get the I2C interface at the given pins, adding it if not yet available.
     */
    inline interfaces::PicoI2C& i2c(unsigned sdaPin, unsigned sclPin) {
        for (auto it = i2cInterfaces().begin(); it != i2cInterfaces().end(); ++it) {
            if (it->sdaPin() == sdaPin && it->sclPin() == sclPin) {
                return *it;
            }
        }
        return i2cInterfaces().at(addInterface(std::move(interfaces::PicoI2C(sdaPin, sclPin))));
    }

#endif

    virtual std::ostream& log() const override {
        return std::cout;
    };
    
    virtual void sleepMs(unsigned ms) const override {
        sleep_ms(ms);
    };
};

} // namespace nl::rakis::raspberrypi