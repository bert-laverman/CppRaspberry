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

#include <cstdint>

#include <iostream>
#include <span>

#include <protocols/messages.hpp>


namespace nl::rakis::raspberrypi::protocols {

    /**
     * @brief Message header
     */
    struct MsgHeader {
        uint8_t command;
        uint8_t length;
        uint8_t sender;
        uint8_t checksum;
    };
    inline constexpr unsigned MsgHeaderSize = sizeof(MsgHeader);

}

namespace nl::rakis::raspberrypi::interfaces
{

class I2C {
    bool verbose_{ false };
    bool initialized_{ false };
    bool listening_{ false };

    unsigned sdaPin_;
    unsigned sclPin_;

    uint8_t address_{ 0 };
    protocols::MsgCallback callback_;

protected:
    inline void initialized(bool initialized) { initialized_ = initialized; }
    inline void listening(bool listening) { listening_ = listening; }
    inline void sdaPin(unsigned sdaPin) { sdaPin_ = sdaPin; }
    inline void sclPin(unsigned sclPin) { sclPin_ = sclPin; }
    inline void listenAddress(uint8_t address) { address_ = address; }
    inline void callback(protocols::MsgCallback callback) { callback_ = callback; }
    
public:
    I2C() = default;
    I2C(unsigned sdaPin, unsigned sclPin) : sdaPin_(sdaPin), sclPin_(sclPin) {}

    I2C(I2C const &) = default;
    I2C(I2C &&) = default;
    I2C &operator=(I2C const &) = default;
    I2C &operator=(I2C &&) = default;
    ~I2C() = default;

    virtual std::ostream &log() = 0;

    inline bool verbose() const { return verbose_; }
    inline void verbose(bool verbose) { verbose_ = verbose; }

    inline bool listening() const { return listening_; }

    /**
        * @brief Check if the I2C bus has been initialized.
        */
    inline bool initialized() const { return initialized_; }

    /**
        * @brief return the GPIO pin number used for the SDA line.
        */
    inline unsigned sdaPin() const { return sdaPin_; }

    /**
        * @brief return the GPIO pin number used for the SCL line.
        */
    inline unsigned sclPin() const { return sclPin_; }

    /**
        * @brief return the I2C address our Pi is using.
        */
    inline uint8_t listenAddress() const { return address_; }

    /**
        * @brief return the callback function used in responder mode.
        */
    inline protocols::MsgCallback callback() { return callback_; }

    /**
        * @brief Open the I2C bus for usage.
        */
    virtual void open() =0;

    /**
        * @brief Close the I2C bus, so it is back in an unintialized state.
        */
    virtual void close() =0;

    /**
        * @brief Cycle the I2C bus back to closed, then immediately open it again.
        */
    inline void reset() { close(); open(); }

    virtual void switchToControllerMode() =0;
    virtual void switchToResponderMode(uint8_t address, protocols::MsgCallback callback) =0;

    virtual bool readBytes(uint8_t address, std::span<uint8_t> data) =0;
    virtual bool writeBytes(uint8_t address, std::span<uint8_t> data) =0;
};

} // namespace nl::rakis::raspberrypi::interfaces