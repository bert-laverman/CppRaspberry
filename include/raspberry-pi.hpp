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

#include <map>
#include <string>
#include <memory>

#include <cstdint>

#include <util/verbose-component.hpp>
#include <interfaces/gpio.hpp>


namespace nl::rakis::raspberrypi::interfaces {
    class I2C;
    class SPI;
} // namespace nl::rakis::raspberrypi::interfaces

namespace nl::rakis::raspberrypi {

class RaspberryPi : public util::VerboseComponent {

#if defined(HAVE_I2C)
    std::map<std::string, std::shared_ptr<interfaces::I2C>> i2c_;
#endif
#if defined(HAVE_SPI)
    std::map<std::string, std::shared_ptr<interfaces::SPI>> spi_;
#endif

public:
    RaspberryPi() = default;
    ~RaspberryPi() = default;

    // There can be only one, so no copying or moving.
    RaspberryPi(RaspberryPi const&) = delete;
    RaspberryPi(RaspberryPi&&) = delete;
    RaspberryPi& operator=(RaspberryPi const&) = delete;
    RaspberryPi& operator=(RaspberryPi&&) = delete;

    /**
     * @brief Return the instance.
     */
    static RaspberryPi& instance();


    /**
     * @brief Sleep for (at least) the given number of milliseconds.
     */
    void sleepMs(unsigned ms) const;

    /**
     * @brief Return a reference to the (local) GPIO interface
     */
    static interfaces::GPIO& gpio();

    operator interfaces::GPIO&() { return gpio(); }

#if defined(HAVE_I2C)

    /**
     * @brief Return the I2C interface with the given name.
     */
    std::shared_ptr<interfaces::I2C> i2c(std::string name) {
        auto it = i2c_.find(name);
        if (it != i2c_.end()) return it->second;
        return std::shared_ptr<interfaces::I2C>();
    }

    /**
     * @brief Add the given I2C interface.
     */
    auto addI2C(std::string name, std::shared_ptr<interfaces::I2C> i2c) {
        i2c_ [name] = i2c;
        return i2c_ [name];
    }

    template <typename S, typename... Args>
    auto addI2C(std::string name, Args&&... args) {
        i2c_ [name] = std::make_shared<S>(std::forward<Args>(args)...);
        return i2c_ [name];
    }

    /**
     * @brief Check if we have an I2C interface at the given pins.
     */
    bool haveI2C(unsigned sdaPin, unsigned sclPin);

#endif

#if defined(HAVE_SPI)

    inline std::shared_ptr<interfaces::SPI> spi(std::string name) {
        auto it = spi_.find(name);
        if (it != spi_.end()) return it->second;
        return std::shared_ptr<interfaces::SPI>();
    }

    auto addSPI(std::string name, std::shared_ptr<interfaces::SPI> spi) {
        spi_ [name] = spi;
        return spi_ [name];
    }

    template <typename S, typename... Args>
    auto addSPI(std::string name, Args&&... args) {
        spi_ [name] = std::make_shared<S>(std::forward<Args>(args)...);
        return spi_ [name];
    }

#endif

};

} // namespace nl::rakis::raspberrypi