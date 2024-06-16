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

#include <components/led.hpp>
#include <interfaces/gpio.hpp>


namespace nl::rakis::raspberrypi::components {


class LocalLed : public Led {
    interfaces::GPIO& gpio_;

    uint pin_;
    bool pullUp_;
    bool state_{false};

    void init() {
        gpio_.init(pin_);
        gpio_.setForOutput(pin_);
        if (pullUp_) {
            gpio_.setPullUp(pin_);
        }
    }

public:
    LocalLed(interfaces::GPIO& gpio, uint pin, bool pullUp = false) : gpio_(gpio), pin_(pin), pullUp_(pullUp) {
        init();
    }

    LocalLed(interfaces::GPIO& gpio, int pin, bool pullUp = false) : gpio_(gpio), pin_(uint(pin)), pullUp_(pullUp) {
        init();
    }

    LocalLed(LocalLed const&) = delete;
    LocalLed(LocalLed&& that) = default;
    LocalLed& operator=(LocalLed const&) = delete;
    LocalLed& operator=(LocalLed&& that) = default;

    virtual ~LocalLed() {
    }

    bool state() const override {
        return state_;
    }

    void set(bool state) override {
        state_ = state;
        gpio_.set(pin_, state_);
    }

};

} // namespace nl::rakis::raspberrypi::components