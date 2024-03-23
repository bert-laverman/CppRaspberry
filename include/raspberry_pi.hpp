#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the Raspberry Pi

#include <iostream>

#include <cstdint>

#include <interfaces/spi.hpp>
#include <interfaces/i2c.hpp>

namespace nl::rakis::raspberrypi {

class RaspberryPi {
    static RaspberryPi* instance_;

    bool verbose_{false};

public:
    RaspberryPi() = default;
    virtual ~RaspberryPi() = default;
    RaspberryPi(RaspberryPi const&) = delete;
    RaspberryPi(RaspberryPi&&) = default;
    RaspberryPi& operator=(RaspberryPi const&) = delete;
    RaspberryPi& operator=(RaspberryPi&&) = default;

    RaspberryPi(bool verbose) : verbose_(verbose) {}

    static RaspberryPi *instance(bool verbose = false);

    virtual interfaces::SPI& spi(unsigned num = 0) = 0;
    virtual interfaces::I2C& i2c(unsigned num = 0) = 0;

    virtual std::ostream& log() = 0;
    inline bool verbose() const { return verbose_; }
    inline void verbose(bool verbose) { verbose_ = verbose; }

    virtual void sleepMs(unsigned ms) = 0;
};

} // namespace nl::rakis::raspberrypi