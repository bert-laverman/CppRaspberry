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


namespace nl::rakis::raspberrypi::components {

class Led
{
public:
    Led() = default;
    Led(const Led&) = delete;
    Led(Led&&) = default;
    Led& operator=(const Led&) = delete;
    Led& operator=(Led&&) = default;

    virtual ~Led() = default;

    /**
     * @brief Get the current state of the LED.
     */
    virtual bool state() const = 0;

    /**
     * @brief Set the current state of the LED.
     */
    virtual void set(bool state) = 0;

    /**
     * @brief Turn the LED on.
     */
    inline void on() { set(true); }

    /**
     * @brief Turn the LED off.
     */
    inline void off() { set(false); }

    /**
     * @brief Toggle the current state of the LED.
     */
    inline void toggle() { set(!state()); }
};

} // namespace nl::rakis::raspberrypi::components