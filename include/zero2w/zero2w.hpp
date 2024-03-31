#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: General Pico management code

#include <cstdint>
#include <chrono>
#include <thread>

#include <raspberry_pi.hpp>
#include <zero2w/interfaces/zero2w_spi.hpp>
#include <zero2w/interfaces/zero2w_i2c.hpp>


namespace nl::rakis::raspberrypi {

class Zero2W : public virtual RaspberryPi {
    interfaces::zero2w::Zero2WSPI spi0_;
    interfaces::zero2w::Zero2WI2C i2c0_;

public:
    Zero2W(bool verbose =false) : RaspberryPi(verbose), spi0_(),i2c0_() {
        spi0_.verbose(verbose);
        i2c0_.verbose(verbose);
    };

    virtual interfaces::SPI& spi([[maybe_unused]] unsigned num = 0) {
        return spi0_;
    };
    virtual interfaces::I2C& i2c([[maybe_unused]] unsigned num = 0) {
        return i2c0_;
    };

    virtual std::ostream& log() {
        return std::cerr;
    };

    virtual void sleepMs(unsigned ms) const override {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    };
};

} // namespace nl::rakis::raspberrypi