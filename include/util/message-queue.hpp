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

#include <list>
#include <tuple>
#include <vector>
#include <functional>

#include <util/verbose-component.hpp>
#include <protocols/messages.hpp>


namespace nl::rakis::raspberrypi::util {

class MessageQueue : public VerboseComponent {
public:
    using Handler = std::function<void(protocols::Command command, uint8_t sender, const std::vector<uint8_t>& data)>;
    using Queue = std::list<std::tuple<protocols::Command, uint8_t, std::vector<uint8_t>>>;

private:
    Queue queue_;

public:
    MessageQueue() = default;
    MessageQueue(const MessageQueue&) = default;
    MessageQueue(MessageQueue&&) = default;
    ~MessageQueue() = default;

    MessageQueue& operator=(const MessageQueue&) = default;
    MessageQueue& operator=(MessageQueue&&) = default;

    /**
     * @brief check if there are no messages in the queue.
     */
    virtual bool empty() {
        return queue_.empty();
    }

    /**
     * @brief Check if there are messages in the queue.
     */
    inline bool haveMessages() {
        return !empty();
    }

    /**
     * @brief Add a message to the queue.
     */
    virtual void push(protocols::Command command, uint8_t address, std::span<uint8_t> data) {
        queue_.push_back({command, address, std::vector<uint8_t>(data.begin(), data.end())});
    }

    /**
     * @brief Get the next message from the queue and return true if there was one.
     */
    [[nodiscard]]
    virtual bool pop(protocols::Command& command, uint8_t& address, std::vector<uint8_t>& data) {
        auto result = !queue_.empty();
        bool error{ false };

        try {
            if (result) {
                std::tie(command, address, data) = queue_.front();
                queue_.pop_front();
            }
        }
        catch (...) {
            error = true;
        }
        if (error) {
            log("Error trying to remove a message from the 'incoming' queue.");
            result = false;
        }
        return result;
    }

    /**
     * @brief Convenience method for emptying the queue and calling a given function on each.
     */
    void processAll(Handler handle) {
        protocols::Command command{ protocols::Command::Hello };
        uint8_t sender{ 0x00 };
        std::vector<uint8_t> data;

        while (pop(command, sender, data)) {
            handle(command, sender, data);
        }
    }

};

} // namespace nl::rakis::raspberrypi::util