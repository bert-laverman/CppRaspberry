#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the SPI bus

#include <cstdint>

#include <array>
#include <functional>

#include "hardware/spi.h"

#include <interfaces/spi.hpp>

namespace nl::rakis::raspberry::interfaces
{

    enum class DefaultPin : unsigned
    {
        SPI0_MISO = 16,
        SPI0_CS = 17,
        SPI0_SCK = 18,
        SPI0_MOSI = 19
    };

    inline consteval unsigned operator+(DefaultPin pin)
    {
        return unsigned(pin);
    };

    class PicoSPI : public virtual SPI
    {
        spi_inst_t *interface_;

        unsigned csPin_;
        unsigned sckPin_;
        unsigned mosiPin_;
        unsigned misoPin_;

        unsigned num_modules_{1}; // Number of devices daisy-chained

    public:
        PicoSPI(spi_inst_t *interface, unsigned csPin, unsigned sckPin, unsigned mosiPin, unsigned misoPin)
            : SPI(), interface_(interface), csPin_(csPin), sckPin_(sckPin), mosiPin_(mosiPin), misoPin_(misoPin)
        {
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
        }

        PicoSPI(unsigned csPin, unsigned sckPin, unsigned mosiPin, unsigned misoPin) : PicoSPI(spi0, csPin, sckPin, mosiPin, misoPin)
        {
        }

        PicoSPI() : PicoSPI(+DefaultPin::SPI0_CS, +DefaultPin::SPI0_SCK, +DefaultPin::SPI0_MOSI, +DefaultPin::SPI0_MISO)
        {
        }

    public:
        virtual void select()
        {
            asm volatile("nop \n nop \n nop");
            gpio_put(csPin_, 0);
            asm volatile("nop \n nop \n nop");
        }

        virtual void deselect()
        {
            asm volatile("nop \n nop \n nop");
            gpio_put(csPin_, 1);
            asm volatile("nop \n nop \n nop");
        }

        virtual void writeAll(std::array<uint8_t, 2> const &value)
        {
            select();
            for (unsigned module = 0; module < num_modules_; ++module)
            {
                spi_write_blocking(interface_, value.data(), 2);
            }
            deselect();
        }

        virtual void writeAll(std::function<std::array<uint8_t, 2>(unsigned)> const &value)
        {
            select();
            for (unsigned module = 0; module < num_modules_; ++module)
            {
                spi_write_blocking(interface_, value(module).data(), 2);
            }
            deselect();
        }
    };

} // namespace nl::rakis::raspberry::interfaces