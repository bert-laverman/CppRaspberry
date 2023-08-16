#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: General Pico management code

#include <pico/stdlib.h>

#include <pico/interfaces/pico_spi.hpp>

namespace nl::rakis::raspberry {

class RaspberryPi;

class PICO : public virtual RaspberryPi {
    interfaces::PicoSPI spi0_;

public:
    PICO() : spi0_() {
        stdio_init_all();
    };

    virtual interfaces::SPI& spi([[maybe_unused]] unsigned num = 0) {
        return spi0_;
    };
};

} // namespace nl::rakis::raspberry