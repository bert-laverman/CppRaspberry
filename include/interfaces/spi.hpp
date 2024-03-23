#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the SPI bus

#include <cstdint>

#include <array>
#include <functional>
#include <iostream>

namespace nl::rakis::raspberrypi::interfaces
{

    class SPI
    {
        unsigned num_modules_{1}; // Number of devices daisy-chained
        bool verbose_{false};

    protected:
        SPI() = default;
        SPI(SPI const &) = default;
        SPI(SPI &&) = default;
        SPI &operator=(SPI const &) = default;
        SPI &operator=(SPI &&) = default;

        virtual std::ostream &log() = 0;

    public:
        virtual ~SPI() = default;

        inline unsigned numModules() const { return num_modules_; }
        inline void numModules(unsigned num_modules) { num_modules_ = num_modules; }

        inline bool verbose() const { return verbose_; }
        inline void verbose(bool verbose) { verbose_ = verbose; }

        virtual void select() =0;

        virtual void deselect() =0;

        virtual void writeAll(std::array<uint8_t, 2> const &value) =0;

        virtual void writeAll(std::function<std::array<uint8_t, 2>(unsigned)> const &value) =0;
    };

} // namespace nl::rakis::raspberrypi::interfaces