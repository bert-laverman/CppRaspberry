#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the I2C bus

#include <cstdint>
#include <span>

namespace nl::rakis::raspberrypi::interfaces {

    class I2C {
    protected:
        I2C() = default;
        I2C(I2C const &) = default;
        I2C(I2C &&) = default;
        I2C &operator=(I2C const &) = default;
        I2C &operator=(I2C &&) = default;

    public:
        virtual ~I2C() = default;

        virtual bool write(uint8_t address, std::span<uint8_t> value) =0;
    };
}