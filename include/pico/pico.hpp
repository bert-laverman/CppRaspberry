#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: General Pico management code

#include "pico/stdlib.h"
#include "SPI.hpp"

namespace nl::rakis::raspberry {

class PICO {
    interfaces::SPI spi0_;

public:
    PICO() : spi0_() {
        stdio_init_all();
    };

    inline interfaces::SPI& spi([[maybe_unused]] uint num = 0) {
        return spi0_;
    };
};

} // namespace nl::rakis::raspberry