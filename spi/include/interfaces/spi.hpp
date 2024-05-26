#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the SPI bus

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