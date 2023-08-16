#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the SPI bus

#include <cstdint>

#include <array>
#include <functional>

namespace nl::rakis::raspberry::interfaces
{

    class SPI
    {

        uint csPin_;
        uint sckPin_;
        uint mosiPin_;
        uint misoPin_;

        uint num_modules_{1}; // Number of devices daisy-chained

    protected:
        SPI(uint csPin, uint sckPin, uint mosiPin, uint misoPin)
            : csPin_(csPin), sckPin_(sckPin), mosiPin_(mosiPin), misoPin_(misoPin)
        {
        }

    public:
        virtual ~SPI() = default;

        inline void numModules(uint num_modules)
        {
            num_modules_ = num_modules;
        }
        inline uint numModules() const
        {
            return num_modules_;
        }

        virtual void select() =0;

        virtual void deselect() =0;

        virtual void open() =0;

        virtual void writeAll(std::array<uint8_t, 2> const &value) =0;

        virtual void writeAll(std::function<std::array<uint8_t, 2>(uint)> const &value) =0;
    };

} // namespace nl::rakis::raspberry::interfaces