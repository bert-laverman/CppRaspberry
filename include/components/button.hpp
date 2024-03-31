#pragma once
// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: A simble pushbutton

#include <pico/stdlib.h>


namespace nl::rakis::raspberrypi::components {

class Button {
    uint pin_;

public:
    Button(uint pin) : pin_(pin) {
        gpio_init(pin_);
        gpio_set_dir(pin_, GPIO_IN);
        gpio_pull_up(pin_);
    }
    virtual ~Button() = default;
};

} // namespace nl::rakis::raspberrypi::components