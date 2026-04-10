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

#include <string>
#include <cstdint>


namespace nl::rakis::raspberrypi {

namespace interfaces { class GPIO; } // Forward declaration


/**
 * The RaspberryPi class is a singleton that provides access to the Raspberry Pi's GPIO interface
 * and a few utility functions.
 */
class RaspberryPi {

public:
    RaspberryPi();
    ~RaspberryPi() = default;

    // There can be only one, so no copying or moving.
    RaspberryPi(RaspberryPi const&) = delete;
    RaspberryPi(RaspberryPi&&) = delete;
    RaspberryPi& operator=(RaspberryPi const&) = delete;
    RaspberryPi& operator=(RaspberryPi&&) = delete;

    /**
     * Return the instance.
     */
    static RaspberryPi& instance();

    /**
     * Log the provided string.
     * 
     * @param s The string to log.
     * @param addNewline If true, a newline is added to the log message.
     */
    static void log(std::string const& s, bool addNewline = true);

    /**
     * Sleep for (at least) the given number of milliseconds.
     * 
     * @param ms The number of milliseconds to sleep.
     */
    void sleepMs(unsigned ms) const;

    /**
     * Return a reference to the (local) GPIO interface
     */
    static interfaces::GPIO& gpio();

    /**
     * Return a reference to the (local) GPIO interface
     */
    operator interfaces::GPIO&() { return gpio(); }

};

} // namespace nl::rakis::raspberrypi