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

#include <raspberry-pi.hpp>
#include <interfaces/pico-spi.hpp>


using namespace nl::rakis::raspberrypi::interfaces;




void PicoSPI::doOpen()
{
    if (initialized()) {
        return;
    }

    if (verbose()) {
        printf("Claiming pins for SPI bus %d: MOSI=%d, MISO=%d, CLK=%d, and CS=%d.\n", busNr_, mosiPin(), misoPin(), sclkPin(), csPin());
    }
    auto& gpio = RaspberryPi::gpio();
    gpio.init(csPin());
    gpio.setForOutput(csPin());
    // gpio.pullUp(csPin());

    gpio.init(sclkPin(), GPIOMode::SPI);
    gpio.init(mosiPin(), GPIOMode::SPI);
    gpio.init(misoPin(), GPIOMode::SPI);

    if (verbose()) {
        printf("Initializing SPI interface %d at %d BAUD.\n", busNr_, baudRate());
    }
    spi_init(interface_, baudRate());

    initialized(true);
}

void PicoSPI::doClose()
{
    if (verbose()) { printf("Closing SPI interface\n"); }
}

void PicoSPI::doWrite(const std::span<uint8_t> data)
{
    open();

    if (verbose()) {
        printf("Writing: %d bytes.\n", data.size());
    }
    select();
    spi_write_blocking(interface_, data.data(), data.size());
    deselect();
}
