#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the SPI bus

#include <cstdint>

#include <array>
#include <functional>

#if defined(TARGET_PICO)

#include "hardware/spi.h"

#else

#include <string>

#endif

namespace nl::rakis::raspberry::interfaces
{

#if !defined(TARGET_PICO)

    using spi_inst_t = const char;

    constexpr static spi_inst_t *spi0{"/dev/spidev0.0"};

#endif

    enum class DefaultPin : uint
    {
#if defined(TARGET_PICO)
        SPI0_MISO = 16,
        SPI0_CS = 17,
        SPI0_SCK = 18,
        SPI0_MOSI = 19
#else
        SPI0_MISO = 9,
        SPI0_CS = 8,
        SPI0_SCK = 11,
        SPI0_MOSI = 10
#endif        
    };

    inline consteval uint operator+(DefaultPin pin)
    {
        return uint(pin);
    };

    class SPI
    {
        spi_inst_t *interface_;

        uint csPin_;
        uint sckPin_;
        uint mosiPin_;
        uint misoPin_;

        uint num_modules_{1}; // Number of devices daisy-chained

    public:
        SPI(spi_inst_t *interface, uint csPin, uint sckPin, uint mosiPin, uint misoPin)
            : interface_(interface), csPin_(csPin), sckPin_(sckPin), mosiPin_(mosiPin), misoPin_(misoPin)
        {
#if defined(TARGET_PICO)
            spi_init(interface_, 10*1000*1000);

            gpio_init(csPin_);
            gpio_set_dir(csPin_, GPIO_OUT);
            gpio_init(sckPin_);
            gpio_set_dir(sckPin_, GPIO_OUT);
            gpio_init(mosiPin_);
            gpio_set_dir(mosiPin_, GPIO_OUT);

            gpio_set_function(csPin_, GPIO_FUNC_SPI);
            gpio_set_function(sckPin_, GPIO_FUNC_SPI);
            gpio_set_function(mosiPin_, GPIO_FUNC_SPI);
            gpio_set_function(misoPin_, GPIO_FUNC_SPI);

            gpio_pull_up(csPin_);
            gpio_pull_up(sckPin_);
            gpio_pull_up(mosiPin_);
            gpio_pull_up(misoPin_);
#else
#endif
        }

        SPI(uint csPin, uint sckPin, uint mosiPin, uint misoPin) : SPI(spi0, csPin, sckPin, mosiPin, misoPin)
        {
        }

        SPI() : SPI(+DefaultPin::SPI0_CS, +DefaultPin::SPI0_SCK, +DefaultPin::SPI0_MOSI, +DefaultPin::SPI0_MISO)
        {
        }

        inline void numModules(uint num_modules)
        {
            num_modules_ = num_modules;
        }
        inline uint numModules() const
        {
            return num_modules_;
        }

        inline void select()
        {
            asm volatile("nop \n nop \n nop");
            gpio_put(csPin_, 0);
            asm volatile("nop \n nop \n nop");
        }

        inline void deselect()
        {
            asm volatile("nop \n nop \n nop");
            gpio_put(csPin_, 1);
            asm volatile("nop \n nop \n nop");
        }

        inline void write(std::array<uint8_t, 2> const &value)
        {
            select();
            spi_write_blocking(interface_, value.data(), 2);
            deselect();
        }

        inline void writeAll(std::array<uint8_t, 2> const &value)
        {
            select();
            for (uint module = 0; module < num_modules_; ++module)
            {
                spi_write_blocking(interface_, value.data(), 2);
            }
            deselect();
        }

        inline void writeAll(std::function<std::array<uint8_t, 2>(uint)> const &value)
        {
            select();
            for (uint module = 0; module < num_modules_; ++module)
            {
                spi_write_blocking(interface_, value(module).data(), 2);
            }
            deselect();
        }
    };

} // namespace nl::rakis::raspberry::interfaces