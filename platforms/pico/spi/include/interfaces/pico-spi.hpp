#pragma once
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


#include <cstdint>

#include <array>
#include <functional>

#include <pico/stdlib.h>
#include <hardware/spi.h>

#include <interfaces/spi.hpp>

namespace nl::rakis::raspberrypi::interfaces
{


inline spi_inst_t* default_spi(int busNr) { return (busNr == 0) ? spi0 : spi1; }


class PicoSPI : public SPI<PicoSPI>
{
    bool initialized_{ false };
    int busNr_;
    spi_inst_t *interface_;

    void initialized(bool init) { initialized_ = init; }

public:
    PicoSPI()
     : PicoSPI(PICO_DEFAULT_SPI, PICO_DEFAULT_SPI_CSN_PIN, PICO_DEFAULT_SPI_SCK_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_RX_PIN)
     {}
    PicoSPI(int busNr, int csPin, int sclkPin, int mosiPin)
     : SPI(csPin, sclkPin, mosiPin), busNr_(busNr), interface_(default_spi(busNr_))
     {}
    PicoSPI(int busNr, int csPin, int sclkPin, int mosiPin, int misoPin)
     : SPI(csPin, sclkPin, mosiPin, misoPin), busNr_(busNr), interface_(default_spi(busNr_))
     {}

    PicoSPI(PicoSPI &&) = default;
    PicoSPI &operator=(PicoSPI &&) = default;

    PicoSPI(const PicoSPI &) = delete;
    PicoSPI &operator=(const PicoSPI &) = delete;

    ~PicoSPI() { doClose(); };

    bool initialized() const { return initialized_; }

    void doOpen();

    void doClose();

    void doWrite(const std::span<uint8_t> data);

};

} // namespace nl::rakis::raspberrypi::interfaces