#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: General Pico management code

#include <cstdint>
#include <chrono>
#include <thread>

#include <raspberry_pi.hpp>
#include "interfaces/zero2w_spi.hpp"


namespace nl::rakis::raspberry {

class Zero2W : public virtual RaspberryPi {
    interfaces::zero2w::Zero2WSPI spi0_;

public:
    Zero2W() : spi0_() {
    };

    virtual interfaces::SPI& spi([[maybe_unused]] unsigned num = 0) {
        return spi0_;
    };
    virtual void sleepMs(unsigned ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    };
};

} // namespace nl::rakis::raspberry