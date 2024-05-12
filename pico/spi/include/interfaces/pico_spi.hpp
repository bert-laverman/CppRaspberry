#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the SPI bus

#include <cstdint>

#include <array>
#include <functional>

#include "pico/stdlib.h"
#include "hardware/spi.h"

#include <interfaces/spi.hpp>

namespace nl::rakis::raspberrypi::interfaces
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

    public:
        PicoSPI(spi_inst_t *interface, unsigned csPin, unsigned sckPin, unsigned mosiPin, unsigned misoPin)
            : interface_(interface), csPin_(csPin), sckPin_(sckPin), mosiPin_(mosiPin), misoPin_(misoPin)
        {
            spi_init(interface_, 10*1000*1000);

            gpio_init(csPin_);
            gpio_set_dir(csPin_, GPIO_OUT);

            gpio_set_function(sckPin_, GPIO_FUNC_SPI);
            gpio_set_function(mosiPin_, GPIO_FUNC_SPI);
            gpio_set_function(misoPin_, GPIO_FUNC_SPI);
        }

        PicoSPI(unsigned csPin, unsigned sckPin, unsigned mosiPin, unsigned misoPin) : PicoSPI(spi0, csPin, sckPin, mosiPin, misoPin)
        {
        }

        PicoSPI() : PicoSPI(+DefaultPin::SPI0_CS, +DefaultPin::SPI0_SCK, +DefaultPin::SPI0_MOSI, +DefaultPin::SPI0_MISO)
        {
        }

        PicoSPI(const PicoSPI &) = default;
        PicoSPI(PicoSPI &&) = default;
        PicoSPI &operator=(const PicoSPI &) = default;
        PicoSPI &operator=(PicoSPI &&) = default;

        virtual ~PicoSPI() = default;

    protected:
        virtual std::ostream &log() {
            return std::cout;
        }

    public:
        virtual void select() override;

        virtual void deselect() override;

        virtual void writeAll(std::array<uint8_t, 2> const &value) override;

        virtual void writeAll(std::function<std::array<uint8_t, 2>(unsigned)> const &value) override;
    };

} // namespace nl::rakis::raspberrypi::interfaces