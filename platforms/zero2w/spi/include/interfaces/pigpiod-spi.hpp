#pragma once
/*
 * Copyright (c) 2023-2024 by Bert Laverman. All Rights Reserved.
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

#include <array>
#include <functional>
#include <string>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include <interfaces/spi.hpp>

namespace nl::rakis::raspberrypi::interfaces
{


/**
    * @brief This class implements a SPI interface using pigpiod.
    */
class PigpiodSPI : public SPI<PigpiodSPI>
{
    int busNr_{ -1 };
    int csNr_{ -1 };

    std::string interface_;

    int channel_{ -1 };
    int fd_{ -1 };

public:
    PigpiodSPI(int busNr, int csNr =0)
        : SPI<PigpiodSPI>(), busNr_(busNr), csNr_(csNr) {}
    PigpiodSPI(int busNr, int csPin, int sclkPin, int mosiPin)
        : SPI<PigpiodSPI>(csPin, sclkPin, mosiPin), busNr_{busNr} {}
    PigpiodSPI(int busNr, int csPin, int sclkPin, int mosiPin, int misoPin)
        : SPI<PigpiodSPI>(csPin, sclkPin, mosiPin, misoPin), busNr_{ busNr } {}
    PigpiodSPI() : PigpiodSPI(0, 0) {}

    PigpiodSPI(PigpiodSPI const &) = default;
    PigpiodSPI(PigpiodSPI &&) = default;
    PigpiodSPI &operator=(PigpiodSPI const &) = default;
    PigpiodSPI &operator=(PigpiodSPI &&) = default;

    ~PigpiodSPI() { doClose(); }

    int busNr() const noexcept { return busNr_; }

    void doOpen();

    void doClose();

    void doWrite(const std::span<uint8_t> data);

};

} // namespace nl::rakis::raspberrypi::interfaces::zero2w