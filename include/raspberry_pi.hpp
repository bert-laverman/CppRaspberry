#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the Raspberry Pi


#include <cstdint>

#include <interfaces/spi.hpp>

namespace nl::rakis::raspberry {

class RaspberryPi {
    static RaspberryPi* instance_;

public:
    RaspberryPi() = default;
    virtual ~RaspberryPi() = default;
    RaspberryPi(RaspberryPi const&) = delete;
    RaspberryPi(RaspberryPi&&) = default;
    RaspberryPi& operator=(RaspberryPi const&) = delete;
    RaspberryPi& operator=(RaspberryPi&&) = default;

    inline static RaspberryPi *instance() {
        return instance_;
    }

    virtual interfaces::SPI& spi(unsigned num = 0) = 0;
};

} // namespace nl::rakis::raspberry