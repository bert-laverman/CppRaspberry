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


#include <pico/sync.h>

#include <util/message-queue.hpp>


namespace nl::rakis::raspberrypi::util {

class PicoMessageQueue : public MessageQueue
{
    critical_section_t section_;

public:
    PicoMessageQueue() : MessageQueue() { critical_section_init(&section_); }
    PicoMessageQueue(const PicoMessageQueue&) = default;
    PicoMessageQueue(PicoMessageQueue&&) = default;
    ~PicoMessageQueue() = default;

    PicoMessageQueue& operator=(const PicoMessageQueue&) = default;
    PicoMessageQueue& operator=(PicoMessageQueue&&) = default;

    virtual bool empty() override {
        critical_section_enter_blocking(&section_);
        auto result = MessageQueue::empty();
        critical_section_exit(&section_);

        return result;
    }

    [[nodiscard]]
    virtual bool pop(protocols::Command& command, uint8_t& address, std::vector<uint8_t>& data) override {
        critical_section_enter_blocking(&section_);
        auto result = MessageQueue::pop(command, address, data);
        critical_section_exit(&section_);

        return result;
    }
};

} // namespace nl::rakis::raspberrypi::util