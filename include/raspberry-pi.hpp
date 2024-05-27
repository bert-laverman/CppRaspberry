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

#include <vector>
#include <iostream>

#include <cstdint>

namespace nl::rakis::raspberrypi::interfaces {
    class I2C;
    class SPI;
} // namespace nl::rakis::raspberrypi::interfaces

namespace nl::rakis::raspberrypi {

template <class Logger
#if defined(HAVE_I2C)
        , class I2CImpl
#endif
#if defined(HAVE_SPI)
        , class SPIImpl
#endif
>
class RaspberryPi {

#if defined(HAVE_I2C)
    std::vector<I2CImpl> i2c_;
#endif
#if defined(HAVE_SPI)
    std::vector<SPIImpl> spi_;
#endif

    bool verbose_{false};

protected:
#if defined(HAVE_I2C)
    inline auto i2cInterfaces() { return i2c_; }
#endif
#if defined(HAVE_SPI)
    inline auto spiInterfaces() { return spi_; }
#endif

public:
    RaspberryPi() = default;
    virtual ~RaspberryPi() = default;
    RaspberryPi(RaspberryPi const&) = default;
    RaspberryPi(RaspberryPi&&) = default;
    RaspberryPi& operator=(RaspberryPi const&) = default;
    RaspberryPi& operator=(RaspberryPi&&) = default;

    RaspberryPi(bool verbose) : verbose_(verbose) {}

    virtual std::ostream& log() const = 0;

    inline bool verbose() const { return verbose_; }
    inline void verbose(bool verbose) { verbose_ = verbose; }

    virtual void sleepMs(unsigned ms) const = 0;

#if defined(HAVE_I2C)

    inline I2CImpl& i2c(unsigned num = 0) { return i2c_[num]; }

    inline auto addInterface(I2CImpl const& i2c) {
        auto num = i2c_.size();
        i2c_.push_back(i2c);
        return num;
    }
    inline auto addInterface(I2CImpl && i2c) {
        auto num = i2c_.size();
        i2c_.push_back(std::move(i2c));
        return num;
    }
#endif

#if defined(HAVE_SPI)
    inline SPIImpl& spi(unsigned num = 0) { return spi_[num]; }

    inline auto addInterface(SPIImpl const& spi) {
        auto num = spi_.size();
        spi_.push_back(spi);
        return num;
    }
    inline auto addInterface(SPIImpl && spi) {
        auto num = spi_.size();
        spi_.push_back(std::move(spi));
        return num;
    }
#endif

};

} // namespace nl::rakis::raspberrypi