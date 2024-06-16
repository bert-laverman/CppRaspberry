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

#include <cstring>

#include <protocols/messages.hpp>
#include <protocols/i2c-protocol-driver.hpp>


namespace nl::rakis::raspberrypi::protocols {

template <typename ProtDriver>
class I2CDeviceHandler {
    ProtDriver& driver_;
    BoardId deviceId_;
    uint8_t controllerAddress_{ GeneralCallAddress };

public:
    I2CDeviceHandler(ProtDriver& driver, BoardId deviceId) : driver_(driver), deviceId_(deviceId) {}

    I2CDeviceHandler(const I2CDeviceHandler&) = delete;
    I2CDeviceHandler(I2CDeviceHandler&&) = delete;
    I2CDeviceHandler& operator=(const I2CDeviceHandler&) = delete;
    I2CDeviceHandler& operator=(I2CDeviceHandler&&) = delete;

    inline const BoardId& deviceId() const { return deviceId_; }

    inline uint8_t controllerAddress() const { return controllerAddress_; }
    inline void controllerAddress(uint8_t address) { controllerAddress_ = address; }

    /**
     * @brief Handle a Hello broadcast message, if sent by the I2C bus controller.
     */
    void handle(uint8_t sender, const MsgHello& msg) {
        if (msg.boardId.id == ControllerId) {
            // The bus controller sent this to announce its own address.
            controllerAddress(sender);
        }
    }

    /**
     * @brief Send a Hello message to a specific address.
     */
    inline bool sendHello(uint8_t address, BoardId id)
    {
        return driver_.sendMessage(Command::Hello, address, std::span<uint8_t>(&((id.bytes)[0]), idSize));
    }

    /**
     * @brief Send a Hello message as a broadcast.
     */
    inline bool sendHello(BoardId id)
    {
        return sendHello(GeneralCallAddress, id);
    }

    /**
     * @brief Handle a SetAddress broadcast message, if for us. This can also change our listen address.
     */
    void handle(const MsgSetAddress& msg) {
        if ((msg.boardId.id == deviceId_.id) && (msg.address != driver_.listenAddress())) {
            driver_.disableResponderMode();
            driver_.enableResponderMode(msg.address);
        }
    }

    /**
     * @brief Send a SetAddress message as a broadcast.
     */
    inline bool sendSetAddress(BoardId id, uint8_t address)
    {
        std::array<uint8_t, sizeMsgSetAddress> msg;
        std::memcpy(msg.data(), &id.bytes[0], idSize);
        msg[idSize] = address;

        return driver_.sendMessage(GeneralCallAddress, Command::SetAddress, msg);
    }

    inline bool haveController() const { return controllerAddress_ != GeneralCallAddress; }
    inline bool needAddress() const { return driver_.listenAddress() == GeneralCallAddress; }

    /**
     * @brief A non-bus controller can call this to request an address from the bus controller.
     */
    void requestAddressIfNeeded() {
        if (haveController() && needAddress()) {
            sendHello(controllerAddress_, deviceId_);
        }
    }

    inline void registerAsDevice() {
        driver_.registerHandler(Command::Hello, "MsgHello handler",
                                [this]([[maybe_unused]] Command command, uint8_t sender, const std::vector<uint8_t>& data) {
            if (data.size() != sizeMsgHello) {
                if (driver_.i2cIn().verbose()) {
                    driver_.i2cIn().log() << "Dropping Hello message: size " << data.size() << " does not match expected " << sizeMsgHello << ".\n";
                }
                return;
            }
            MsgHello msg;
            std::memcpy(&msg.boardId.bytes[0], data.data(), idSize);

            handle(sender, msg);
        });

        driver_.registerHandler(Command::SetAddress, "MsgSetAddress handler",
                                [this]([[maybe_unused]] Command command, [[maybe_unused]] uint8_t sender, const std::vector<uint8_t>& data) {
            if (data.size() != sizeMsgSetAddress) {
                if (driver_.i2cIn().verbose()) {
                    driver_.i2cIn().log() << "Dropping Hello message: size " << data.size() << " does not match expected " << sizeMsgSetAddress << ".\n";
                }
                return;
            }
            MsgSetAddress msg;
            std::memcpy(&msg.boardId.bytes[0], data.data(), idSize);
            msg.address = data[idSize];
            handle(msg);
        });
    }
};

} // namespace nl::raklis::raspberrypi::protocols