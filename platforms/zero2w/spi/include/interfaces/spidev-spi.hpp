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

constexpr static const char *spi0_0{"/dev/spidev0.0"};
constexpr static const char *spi0_1{"/dev/spidev0.1"};


/**
    * @brief This class implements a SPI interface using "/dev/spidev*".
    */
class SPIDevSPI : public SPI
{
    int busNr_;
    int csNr_;

    std::string interface_;

    int fd_{ -1 };
    int channel_{ -1 };

protected:

    void writeBlocking(std::span<uint8_t> data);

    void validate();

public:
    SPIDevSPI(int busNr, int csNr)
        : busNr_(busNr), csNr_(csNr)
    {
        validate();
    }

    SPIDevSPI(const char *interface)
        : interface_(interface)
    {
        validate();
    }

    SPIDevSPI() : SPIDevSPI(spi0_0)
    {
    }

    SPIDevSPI(SPIDevSPI const &) = default;
    SPIDevSPI(SPIDevSPI &&) = default;
    SPIDevSPI &operator=(SPIDevSPI const &) = default;
    SPIDevSPI &operator=(SPIDevSPI &&) = default;

    virtual ~SPIDevSPI() {}

    virtual operator bool() const noexcept override { return fd_ >= 0; }

    virtual void open() override;

    virtual void close() override;

    virtual bool selected() const noexcept override;

    virtual void select() override;

    virtual void deselect() override;

    virtual void write(std::span<uint8_t> value) override;

};

} // namespace nl::rakis::raspberrypi::interfaces::zero2w