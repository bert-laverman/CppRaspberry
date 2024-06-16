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


#include <pico/stdlib.h>

#include <raspberry-pi.hpp>
#include <interfaces/gpio.hpp>

using namespace nl::rakis::raspberrypi;


interfaces::GPIO& RaspberryPi::gpio() {
    static interfaces::GPIO gpio_;

    return gpio_;
}

RaspberryPi& RaspberryPi::instance() {
    static RaspberryPi instance;
    return instance;
}

#if defined(HAVE_I2C)

/**
    * @brief Check if an I2C interface is available at the given pins.
    */
bool haveI2C(unsigned sdaPin, unsigned sclPin) {
    for (auto it = i2cInterfaces().begin(); it != i2cInterfaces().end(); ++it) {
        if (it->sdaPin() == sdaPin && it->sclPin() == sclPin) {
            return true;
        }
    }
    return false;
}

/**
    * @brief Get the I2C interface at the given pins, adding it if not yet available.
    */
interfaces::PicoI2C& i2c(unsigned sdaPin, unsigned sclPin) {
    for (auto it = i2cInterfaces().begin(); it != i2cInterfaces().end(); ++it) {
        if (it->sdaPin() == sdaPin && it->sclPin() == sclPin) {
            return *it;
        }
    }
    return i2cInterfaces().at(addInterface(std::move(interfaces::PicoI2C(sdaPin, sclPin))));
}

#endif

void RaspberryPi::sleepMs(unsigned ms) const {
    sleep_ms(ms);
};
