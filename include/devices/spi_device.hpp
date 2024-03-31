#pragma once
// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47


namespace nl::rakis::raspberrypi::interfaces {
    class SPI;
} // namespace nl::rakis::raspberrypi::interfaces


namespace nl::rakis::raspberrypi::devices {

class SPIDevice
{
    interfaces::SPI& spi_;

public:
    SPIDevice(interfaces::SPI& spi) : spi_(spi) {}

    virtual ~SPIDevice() = default;

    inline interfaces::SPI& interface() const { return spi_; }

    virtual void numModulesChanged(unsigned num_modules) = 0;
};

} // namespace nl::rakis::raspberrypi::devices