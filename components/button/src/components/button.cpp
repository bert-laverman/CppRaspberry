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

#include <boards/pico.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>


#include <array>

#include <devices/button.hpp>


using namespace nl::rakis::raspberrypi::devices;

Button::Button(uint pin) : pin_(pin) {
    setup();
}


static std::array<std::function<void(uint, uint32_t)>, 32> gpioRiseCallbacks;
static std::array<std::function<void(uint, uint32_t)>, 32> gpioFallCallbacks;

static void gpioIRQ(uint gpio, uint32_t events) {
    if ((gpio < 32) && ((events & GPIO_IRQ_EDGE_RISE) != 0) && gpioRiseCallbacks[gpio]) {
        gpioRiseCallbacks[gpio](gpio, events);
    } else if ((gpio < 32) && ((events & GPIO_IRQ_EDGE_FALL) != 0) && gpioFallCallbacks[gpio]) {
        gpioFallCallbacks[gpio](gpio, events);
    }
}


void Button::setup() {
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_IN);
    gpio_pull_up(pin_);

    lastTime_ = to_ms_since_boot(get_absolute_time());
    gpio_set_irq_enabled_with_callback(pin_, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, gpioIRQ);

    gpioRiseCallbacks[pin_] = [this](uint gpio, [[maybe_unused]] uint32_t events) {
        if (gpio != pin_) {
            if (log_) { log_("onUp called for the wrong pin\n"); }
            return;
        }
        if (gpio_get(pin_) == 0) {
            return;
        }
        if ((to_ms_since_boot(get_absolute_time()) - lastTime_) > interval_) {
            lastTime_ = to_ms_since_boot(get_absolute_time());

            if (onUp_) { onUp_(); }
        }
    };
    gpioFallCallbacks[pin_] = [this](uint gpio, [[maybe_unused]] uint32_t events) {
        if (gpio != pin_) {
            if (log_) { log_("onDown called for the wrong pin\n"); }
            return;
        }
        if (gpio_get(pin_) != 0) {
            return;
        }
        if ((to_ms_since_boot(get_absolute_time()) - lastTime_) > interval_) {
            lastTime_ = to_ms_since_boot(get_absolute_time());

            if (onDown_) { onDown_(); }
        }
    };
}
