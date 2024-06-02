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

#include <vector>


#include <protocols/protocol-driver.hpp>
#include <protocols/max7219-messages.hpp>
#include <devices/local-max7219.hpp>


namespace nl::rakis::raspberrypi::protocols {

class MAX7219Handler {
    devices::LocalMAX7219& max_;

public:
    MAX7219Handler(devices::LocalMAX7219& max) : max_(max) {}
    MAX7219Handler(const MAX7219Handler&) = delete;
    MAX7219Handler(MAX7219Handler&&) = delete;
    MAX7219Handler& operator=(const MAX7219Handler&) = delete;
    MAX7219Handler& operator=(MAX7219Handler&&) = delete;
    ~MAX7219Handler() {}

    void handle(const MsgMax7219& msg) {
        switch (toMax7219Command(msg.command)) {

        // Display management
        case Max7219Command::Reset:
            max_.reset();
            break;
        case Max7219Command::Shutdown:
            if (msg.module == MsgMax7219::AllModules)
                max_.shutdown();
            else
                max_.shutdown(msg.module);
            break;
        case Max7219Command::Startup:
            if (msg.module == MsgMax7219::AllModules)
                max_.startup();
            else
                max_.startup(msg.module);
            break;
        case Max7219Command::TestDisplay:
            if (msg.module == MsgMax7219::AllModules)
                max_.displayTest(static_cast<uint8_t>(msg.value));
            else
                max_.displayTest(msg.module, static_cast<uint8_t>(msg.value));
            break;

        // Settings related to how stuff shows up
        case Max7219Command::SetBrightness:
            if (msg.module == MsgMax7219::AllModules)
                max_.setBrightness(static_cast<uint8_t>(msg.value));
            else
                max_.setBrightness(msg.module, static_cast<uint8_t>(msg.value));
            break;
        case Max7219Command::SetScanLimit:
            if (msg.module == MsgMax7219::AllModules)
                max_.setScanLimit(static_cast<uint8_t>(msg.value));
            else
                max_.setScanLimit(msg.module, static_cast<uint8_t>(msg.value));
            break;
        case Max7219Command::SetDecodeMode:
            if (msg.module == MsgMax7219::AllModules)
                max_.setDecodeMode(static_cast<uint8_t>(msg.value));
            else
                max_.setDecodeMode(msg.module, static_cast<uint8_t>(msg.value));
            break;

        // Actually putting stuff on the display
        case Max7219Command::ClearDisplay:
            if (msg.module == MsgMax7219::AllModules)
                max_.clear();
            else
                max_.clear(msg.module);
            break;
        case Max7219Command::SetValue:
            max_.setNumber(msg.module, msg.value);
            break;

        // Caching related
        case Max7219Command::SetSendImmediately:
            max_.writeImmediately(msg.value != 0);
            break;
        case Max7219Command::SendBrightness:
            max_.sendBrightness();
            break;
        case Max7219Command::SendScanLimit:
            max_.sendScanLimit();
            break;
        case Max7219Command::SendDecodeMode:
            max_.sendDecodeMode();
            break;
        case Max7219Command::SendBuffer:
            max_.sendBuffer();
            break;
        case Max7219Command::SendData:
            max_.sendData();
            break;
        default:
            // DONOTHING
            break;
        }
    }

    template <typename DriverImpl>
    inline void registerAt(DriverImpl& driver) {
        driver.registerHandler(Command::Max7219, "Handle MAX7219 messages",
                               [this]([[maybe_unused]] Command command, [[maybe_unused]] uint8_t sender, const std::vector<uint8_t>& data) {
            if (data.size() != sizeof(MsgMax7219)) return;
            const MsgMax7219* msg = reinterpret_cast<const MsgMax7219*>(data.data());
            handle(*msg);
        });
    }
};

} // namespace nl::rakis::raspberrypi::protocols