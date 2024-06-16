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


#include <cstdint>

#include <array>
#include <memory>
#include <functional>
#include <iostream>

#include <raspberry-pi.hpp>
#include <devices/spi-device.hpp>

using nl::rakis::raspberrypi::RaspberryPi;
using nl::rakis::raspberrypi::devices::SPIDevice;

namespace nl::rakis::raspberrypi::interfaces {


class SPI
{
    bool verbose_{ false };
    SPIDevice *device_{ nullptr };
    unsigned numModules_{ 1 };

protected:
    SPI() = default;
    SPI(SPI const &) = default;
    SPI(SPI &&) = default;
    SPI &operator=(SPI const &) = default;
    SPI &operator=(SPI &&) = default;

    virtual std::ostream &log() = 0;

public:
    virtual ~SPI() = default;

    inline bool verbose() const { return verbose_; }
    inline void verbose(bool verbose) { verbose_ = verbose; }

    inline SPIDevice *device() const { return device_; }
    inline void device(SPIDevice *device) { device_ = device; }

    inline unsigned numModules() const { return numModules_; }
    void numModules(unsigned numModules);

    virtual void select() =0;

    virtual void deselect() =0;

    virtual void open() =0;
    virtual void close() =0;
    inline void reset() { close(); open(); }

    virtual void writeAll(std::array<uint8_t, 2> const &value) =0;

    virtual void writeAll(std::function<std::array<uint8_t, 2>(unsigned)> const &value) =0;
};

} // namespace nl::rakis::raspberrypi::interfaces