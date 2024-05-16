// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47

#if !defined(TARGET_PICO) && !defined(HAVE_SPI)
#error "This file is for the Pico with SPI enabled only!"
#endif

#include <interfaces/pico_spi.hpp>

using namespace nl::rakis::raspberrypi::interfaces;


PicoSPI::PicoSPI(spi_inst_t *interface, unsigned csPin, unsigned sckPin, unsigned mosiPin, unsigned misoPin)
    : interface_(interface), csPin_(csPin), sckPin_(sckPin), mosiPin_(mosiPin), misoPin_(misoPin)
{
    open();
}

PicoSPI::PicoSPI(unsigned csPin, unsigned sckPin, unsigned mosiPin, unsigned misoPin) : PicoSPI(spi0, csPin, sckPin, mosiPin, misoPin)
{
}

PicoSPI::PicoSPI() : PicoSPI(+DefaultPin::SPI0_CS, +DefaultPin::SPI0_SCK, +DefaultPin::SPI0_MOSI, +DefaultPin::SPI0_MISO)
{
}

void PicoSPI::open()
{
    spi_init(interface_, 10*1000*1000);

    gpio_init(csPin_);
    gpio_set_dir(csPin_, GPIO_OUT);

    gpio_set_function(sckPin_, GPIO_FUNC_SPI);
    gpio_set_function(mosiPin_, GPIO_FUNC_SPI);
    gpio_set_function(misoPin_, GPIO_FUNC_SPI);
}

void PicoSPI::close()
{
    //DONOTHING
}

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