#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: General Pico management code

#include <cstdint>
#include <chrono>
#include <thread>

#include <raspberry_pi.hpp>

#if defined(HAVE_I2C)
#include <zero2w/interfaces/zero2w_i2c.hpp>
#endif
#if defined(HAVE_SPI)
#include <zero2w/interfaces/zero2w_spi.hpp>
#endif


namespace nl::rakis::raspberrypi {

class Zero2W
    : public virtual RaspberryPi<std::ostream
#if defined(HAVE_I2C)
    , interfaces::zero2w::Zero2WI2C
#endif
#if defined(HAVE_SPI)
    , interfaces::zero2w::Zero2WSPI
#endif
    >
{
public:
    Zero2W(bool verbose =false) : RaspberryPi(verbose) {};

    virtual ~Zero2W() =default;

    virtual std::ostream& log() {
        return std::cerr;
    };

    virtual void sleepMs(unsigned ms) const override {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    };
};

} // namespace nl::rakis::raspberrypi