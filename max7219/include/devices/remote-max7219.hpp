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

#include <devices/max7219.hpp>

#include <protocols/i2c-protocol-driver.hpp>
#include <protocols/max7219-messages.hpp>


using namespace nl::rakis::raspberrypi::protocols;

namespace nl::rakis::raspberrypi::devices {

template <typename I2COutImpl, typename I2CInImpl>
class RemoteMAX7219 : public MAX7219 {
    I2CProtocolDriver<I2COutImpl, I2CInImpl>& driver_;
    uint8_t address_;

public:
    RemoteMAX7219(protocols::I2CProtocolDriver<I2COutImpl, I2CInImpl>& driver, uint8_t address) : driver_(driver), address_(address) {}
    virtual ~RemoteMAX7219() = default;

    void sendBrightness() override {
        MsgMax7219 msg{ .command = toValue(Max7219Command::SendBrightness) };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

    void sendScanLimit() override {
        MsgMax7219 msg{ .command = toValue(Max7219Command::SendScanLimit) };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

    void sendDecodeMode() override {
        MsgMax7219 msg{ .command = toValue(Max7219Command::SendDecodeMode) };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

    void sendBuffer() override {
        MsgMax7219 msg{ .command = toValue(Max7219Command::SendBuffer) };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

    void sendData() override {
        MsgMax7219 msg{ .command = toValue(Max7219Command::SendData) };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

    void shutdown() override {
        MsgMax7219 msg{ .command = toValue(Max7219Command::Shutdown) };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

    void shutdown(uint8_t module) override {
        MsgMax7219 msg{ .module =module, .command = toValue(Max7219Command::Shutdown) };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

    void startup() override {
        MsgMax7219 msg{ .command = toValue(Max7219Command::Startup) };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

    void startup(uint8_t module) override {
        MsgMax7219 msg{ .module =module, .command = toValue(Max7219Command::Startup) };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

    void displayTest(uint8_t value) override {
        MsgMax7219 msg{ .command = toValue(Max7219Command::TestDisplay), .value = value };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

    void displayTest(uint8_t module, uint8_t value) override {
        MsgMax7219 msg{ .module =module, .command = toValue(Max7219Command::TestDisplay), .value = value };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

    virtual void clear() override {
        MsgMax7219 msg{ .command = toValue(Max7219Command::ClearDisplay) };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

    virtual void clear(uint8_t module) override {
        MsgMax7219 msg{ .module =module, .command = toValue(Max7219Command::ClearDisplay) };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

    virtual void setNumber(uint8_t module, int32_t value) override {
        MsgMax7219 msg{ .module =module, .command = toValue(Max7219Command::SetValue), .value = value };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

    virtual void reset() override {
        MsgMax7219 msg{ .command = toValue(Max7219Command::Reset) };
        driver_.sendMessage(address_, Command::Max7219, msg);
    }

};

} // namespace nl::rakis::raspberrypi::devices