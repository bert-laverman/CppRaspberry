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


#include <cstdint>
#include <chrono>
#include <thread>
#include <iostream>

#include <raspberry-pi.hpp>
#include <interfaces/gpio.hpp>

using namespace nl::rakis::raspberrypi;


/**
 * Constructor.
 */
RaspberryPi::RaspberryPi() { }


/**
 * Logs a message to the standard error stream.
 *
 * @param s The message to log.
 * @param addNewline If true, a newline character is added after the message.
 */
void RaspberryPi::log(std::string const& s, bool addNewline) {
    if (addNewline) {
        std::cerr << s << std::endl;
    } else {
        std::cerr << s;
    }
}


/**
 * Returns a reference to the GPIO interface. Using a static variable in a function ensures that it is initialized
 * exactly once, when the function is first called.
 * 
 * @return A reference to the GPIO interface.
 */
interfaces::GPIO& RaspberryPi::gpio() {
    static interfaces::GPIO gpio_;

    return gpio_;
}


/**
 * Returns the instance. Using a static variable in a function ensures that it is initialized
 * exactly once, when the function is first called.
 * 
 * @return A reference to the RaspberryPi instance.
 */
RaspberryPi& RaspberryPi::instance() {
    static RaspberryPi instance;

    return instance;
}


/**
 * Sleep for (at least) the given number of milliseconds.
 *
 * @param ms The number of milliseconds to sleep.
 */
void RaspberryPi::sleepMs(unsigned ms) const {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
