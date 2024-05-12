// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47

#if !defined(TARGET_PICO) && !defined(HAVE_SPI)
#error "This file is for the Pico with SPI enabled only!"
#endif

#include <pico/interfaces/pico_spi.hpp>

using namespace nl::rakis::raspberrypi::interfaces;

void PicoSPI::select()
{
    asm volatile("nop \n nop \n nop");
    gpio_put(csPin_, 0);
    asm volatile("nop \n nop \n nop");
}

void PicoSPI::deselect()
{
    asm volatile("nop \n nop \n nop");
    gpio_put(csPin_, 1);
    asm volatile("nop \n nop \n nop");
}

void PicoSPI::writeAll(std::array<uint8_t, 2> const &value)
{
    select();
    for (unsigned module = 0; module < numModules(); ++module)
    {
        spi_write_blocking(interface_, value.data(), 2);
    }
    deselect();
}

void PicoSPI::writeAll(std::function<std::array<uint8_t, 2>(unsigned)> const &value)
{
    select();
    for (unsigned module = 0; module < numModules(); ++module)
    {
        spi_write_blocking(interface_, value(module).data(), 2);
    }
    deselect();
}