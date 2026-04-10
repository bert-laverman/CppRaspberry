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

#include <raspberry-pi.hpp>


namespace nl::rakis::raspberrypi::util {


/**
 * A VerboseComponent can emit logging, if enabled.
 */
class VerboseComponent {
    bool verbose_ { false };

public:
    VerboseComponent() = default;
    ~VerboseComponent() = default;

    VerboseComponent(const VerboseComponent&) = default;
    VerboseComponent(VerboseComponent&&) = default;
    VerboseComponent& operator=(const VerboseComponent&) = default;
    VerboseComponent& operator=(VerboseComponent&&) = default;


    /**
     * Returns if this component should actually produce logging.
     */
    bool verbose() const noexcept { return verbose_; }


    /**
     * Set if this component should actually produce logging.
     */
    void verbose(bool verb) noexcept { verbose_ = verb; }

    /**
     * Convenience method to send the provided string to the log, if in verbose mode.
     * 
     * @param s The string to log.
     * @param addNewline If true, a newline is added to the log message.
     */
    void log(const std::string s, bool addNewline =true) {
        if (verbose()) {
            RaspberryPi::log(s, addNewline);
        }
    }
};

} // namespace nl::rakis::raspberrypi::util