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

#include <pico/stdlib.h>


namespace nl::rakis::raspberrypi::devices {


class Led {
    uint pin_;
    bool pullUp_;
    bool state_{false};

public:
    Led(uint pin, bool pullUp = false) : pin_(pin), pullUp_(pullUp) {
        gpio_init(pin_);
        gpio_set_dir(pin_, GPIO_OUT);
        if (pullUp_) {
            gpio_pull_up(pin_);
        }
    }

    Led(int pin, bool pullUp = false) : pin_(uint(pin)), pullUp_(pullUp) {
        gpio_init(pin_);
        gpio_set_dir(pin_, GPIO_OUT);
        if (pullUp_) {
            gpio_pull_up(pin_);
        }
    }

    virtual ~Led() = default;

    Led(Led const&) = default;
    Led(Led&&) = default;
    Led& operator=(Led const&) = default;
    Led& operator=(Led&&) = default;

    inline bool state() const {
        return state_;
    }

    inline void set(bool state) {
        state_ = state;
        gpio_put(pin_, state_ ? 1 : 0);
    }

    inline void on() {
        set(true);
    }
    inline void off() {
        set(false);
    }
    inline void toggle() {
        set(!state_);
    }
};

} // namespace nl::rakis::raspberrypi::devices