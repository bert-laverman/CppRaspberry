/*
 * Copyright (c) 2024 by Bert Laverman. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#if !defined(TARGET_PICO) && !defined(HAVE_SPI)
#error "This file is for the Pico with SPI enabled only!"
#endif

#include <pico/stdlib.h>

#include <interfaces/pico-spi.hpp>

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
    if (initialized()) {
        return;
    }
    spi_init(interface_, 5*1000*1000);

    if (verbose()) {
        printf("Initializing SPI interface with MOSI=%d, MISO=%d, CLK=%d, and CS=%d.\n", mosiPin_, misoPin_, sckPin_, csPin_);
    }
    gpio_init(csPin_);
    gpio_set_dir(csPin_, GPIO_OUT);

    gpio_set_function(sckPin_, GPIO_FUNC_SPI);
    gpio_set_function(mosiPin_, GPIO_FUNC_SPI);
    gpio_set_function(misoPin_, GPIO_FUNC_SPI);

    initialized(true);
}

void PicoSPI::close()
{
    if (verbose()) { printf("Closing SPI interface\n"); }
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
    if (verbose()) {
        printf("Writing: %d times 0x%02x 0x%02x.\n", numModules(), value[0], value[1]);
    }
    open();

    select();
    for (unsigned module = 0; module < numModules(); ++module)
    {
        spi_write_blocking(interface_, value.data(), 2);
    }
    deselect();
}

void PicoSPI::writeAll(std::function<std::array<uint8_t, 2>(unsigned)> const &value)
{
    if (verbose()) {
        printf("Writing:");
        for (unsigned module = 0; module < numModules(); ++module)
        {
            auto v = value(module);
            printf(" 0x%02x 0x%02x", v[0], v[1]);
        }
        printf("\n");
    }
    open();

    select();
    for (unsigned module = 0; module < numModules(); ++module)
    {
        spi_write_blocking(interface_, value(module).data(), 2);
    }
    deselect();
}