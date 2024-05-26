#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: General Pico management code

#include <cstdint>
#include <chrono>
#include <thread>

#include <raspberry-pi.hpp>

#if defined(HAVE_I2C)
#include <interfaces/zero2w-i2c.hpp>
#endif
#if defined(HAVE_SPI)
#include <interfaces/zero2w-spi.hpp>
#endif


namespace nl::rakis::raspberrypi {

class Zero2W
    : public RaspberryPi<std::ostream
#if defined(HAVE_I2C)
    , interfaces::Zero2WI2C
#endif
#if defined(HAVE_SPI)
    , interfaces::Zero2WSPI
#endif
    >
{
public:
    Zero2W(bool verbose =false) : RaspberryPi(verbose) {};

    virtual ~Zero2W() =default;

    static Zero2W& instance(bool verbose =false) {
        static Zero2W instance{ verbose };
        return instance;
    }

    virtual std::ostream& log() const override {
        return std::cerr;
    };

    virtual void sleepMs(unsigned ms) const override {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    };

};

} // namespace nl::rakis::raspberrypi