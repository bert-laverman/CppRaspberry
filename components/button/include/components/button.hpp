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

#include <string>
#include <functional>


namespace nl::rakis::raspberrypi::components {


using ButtonCallback = std::function<void(void)>;

class Button {
    uint pin_;

    uint32_t lastTime_{ 0 };
    uint32_t interval_{ 50 };

    ButtonCallback onUp_;
    ButtonCallback onDown_;

    std::function<void(std::string)> log_;

protected:
    virtual void setup()/* = 0*/;

public:
    Button(uint pin);
    virtual ~Button() = default;

    Button(const Button&) = default;
    Button(Button&&) = default;
    Button& operator=(const Button&) = default;
    Button& operator=(Button&&) = default;

    inline void onUp(ButtonCallback cb) { onUp_ = cb; }
    inline void onDown(ButtonCallback cb) { onDown_ = cb; }
    inline void onLog(std::function<void(std::string)> log) { log_ = log; }
};

} // namespace nl::rakis::raspberrypi::components