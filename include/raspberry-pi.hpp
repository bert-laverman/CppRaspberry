#pragma once
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

#include <map>
#include <string>
#include <memory>

#include <cstdint>

#include <util/verbose-component.hpp>
#include <interfaces/gpio.hpp>


namespace nl::rakis::raspberrypi {

class RaspberryPi : public util::VerboseComponent {

public:
    RaspberryPi() = default;
    ~RaspberryPi() = default;

    // There can be only one, so no copying or moving.
    RaspberryPi(RaspberryPi const&) = delete;
    RaspberryPi(RaspberryPi&&) = delete;
    RaspberryPi& operator=(RaspberryPi const&) = delete;
    RaspberryPi& operator=(RaspberryPi&&) = delete;

    /**
     * @brief Return the instance.
     */
    static RaspberryPi& instance();


    /**
     * @brief Sleep for (at least) the given number of milliseconds.
     */
    void sleepMs(unsigned ms) const;

    /**
     * @brief Return a reference to the (local) GPIO interface
     */
    static interfaces::GPIO& gpio();

    operator interfaces::GPIO&() { return gpio(); }

};

} // namespace nl::rakis::raspberrypi