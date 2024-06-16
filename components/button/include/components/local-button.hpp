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


#include <string>
#include <functional>

#include <interfaces/gpio.hpp>
#include <components/button.hpp>


namespace nl::rakis::raspberrypi::components {


class LocalButton : public Button {
    interfaces::GPIO& gpio_;

    uint pin_;

    uint32_t lastTime_{ 0 };
    uint32_t interval_{ 50 };

    ButtonCallback onUp_;
    ButtonCallback onDown_;

    // std::function<void(std::string)> log_;

    void init() {
        gpio_.init(pin_);
        gpio_.setForInput(pin_);
        gpio_.setPullUp(pin_);

        gpio_.addRiseHandler(pin_, [this]([[maybe_unused]] uint gpio, [[maybe_unused]] uint32_t events) {
            if (onUp_) { onUp_(); }
        });
        gpio_.addFallHandler(pin_, [this]([[maybe_unused]] uint gpio, [[maybe_unused]] uint32_t events) {
            if (onDown_) { onDown_(); }
        });
    }

public:
    LocalButton(interfaces::GPIO& gpio, uint pin) : gpio_(gpio), pin_(pin) { init(); }

    virtual ~LocalButton() = default;

    LocalButton(const LocalButton&) = default;
    LocalButton(LocalButton&&) = default;
    LocalButton& operator=(const LocalButton&) = default;
    LocalButton& operator=(LocalButton&&) = default;

    virtual void onUp(ButtonCallback cb) override { onUp_ = cb; }
    virtual void onDown(ButtonCallback cb) override { onDown_ = cb; }
    // virtual void onLog(std::function<void(std::string)> log) override { log_ = log; }
};

} // namespace nl::rakis::raspberrypi::components