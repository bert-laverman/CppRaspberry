#pragma once
// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: A simble pushbutton

#include <pico/stdlib.h>


namespace nl::rakis::raspberrypi::components {

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

} // namespace nl::rakis::raspberrypi::components