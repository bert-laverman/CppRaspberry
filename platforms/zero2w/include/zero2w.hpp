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
#include <chrono>
#include <thread>

#include <raspberry-pi.hpp>

#if defined(HAVE_I2C)
#include <interfaces/zero2w-i2c.hpp>
#endif
#if defined(HAVE_SPI)
#include <interfaces/zero2w-spi.hpp>
#endif


namespace nl::rakis::raspberrypi {

class Zero2W
    : public RaspberryPi<std::ostream
#if defined(HAVE_I2C)
    , interfaces::Zero2WI2C
#endif
#if defined(HAVE_SPI)
    , interfaces::Zero2WSPI
#endif
    >
{
public:
    Zero2W(bool verbose =false) : RaspberryPi(verbose) {};

    virtual ~Zero2W() =default;

    static Zero2W& instance(bool verbose =false) {
        static Zero2W instance{ verbose };
        return instance;
    }

    virtual std::ostream& log() const override {
        return std::cerr;
    };

    virtual void sleepMs(unsigned ms) const override {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    };

};

} // namespace nl::rakis::raspberrypi