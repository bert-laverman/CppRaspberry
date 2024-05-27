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

} // namespace nl::rakis::raspberrypi::devices