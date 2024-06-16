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


#include <cstddef>
#include <cstring>
#include <cstdint>

#include <map>
#include <span>
#include <list>
#include <tuple>
#include <vector>

#if defined(TARGET_PICO)
#include <pico/stdlib.h>
#else
#include <format>
#endif

#include <interfaces/i2c.hpp>
#include <protocols/protocol-driver.hpp>


namespace nl::rakis::raspberrypi::protocols {


/**
 * @brief This class manages communication through I2C, using one bus for incoming, and another for outgoing messages.
 */
template <typename I2COutImpl, typename I2CInImpl, typename QueueImpl>
class I2CProtocolDriver : public ProtocolDriver<QueueImpl> {
    I2COutImpl& i2cOut_;
    I2CInImpl& i2cIn_;

public:
    I2CProtocolDriver(I2COutImpl& out, I2CInImpl& in) : i2cOut_(out), i2cIn_(in) {};

    virtual ~I2CProtocolDriver() {}

    I2CProtocolDriver(I2CProtocolDriver const &) = delete;
    I2CProtocolDriver(I2CProtocolDriver &&) = delete;
    I2CProtocolDriver &operator=(I2CProtocolDriver const &) = delete;
    I2CProtocolDriver &operator=(I2CProtocolDriver &&) = delete;

protected:
    /**
     * @brief Reset the I2C protocol driver. If operating in dual mode, set both in their correct modes.
     */
    void reset() {
        i2cOut_.reset();
        i2cOut_.switchToControllerMode();
        i2cIn_.reset();
        i2cIn_.switchToResponderMode(i2cIn_.listenAddress(), i2cIn_.callback());
    }

    /**
     * @brief Compute a (simple) checksum.
     */
    uint8_t computeChecksum(std::span<uint8_t> data) {
        uint8_t checksum = 0;
        for (auto it = data.begin(); it != data.end(); ++it) {
            checksum ^= *it;
        }
        return checksum;
    }

public:

    inline I2CInImpl& i2cIn() { return i2cIn_; }
    inline I2COutImpl& i2cOut() { return i2cOut_; }

    void enableControllerMode() override {
        i2cOut_.switchToControllerMode();
    }

    void disableControllerMode() override {
        i2cOut_.close();
    }

    virtual void enableResponderMode(uint8_t address = GeneralCallAddress) override {
        i2cIn_.switchToResponderMode(address, [this](Command command, uint8_t sender, const std::span<uint8_t> data) {
            this->pushIncoming(command, sender, data);
        });
    }

    bool isListening() const override {
        return i2cIn_.listening();
    }

    uint8_t listenAddress() const override {
        return i2cIn_.listenAddress();
    }

    void disableResponderMode() override {
        i2cIn_.close();
    }

    bool sendMessage(Command command, uint8_t address, const std::span<uint8_t> msg) override {
        std::vector<uint8_t> data(MsgHeaderSize + msg.size(), 0x00);
        std::memcpy(data.data() + MsgHeaderSize, msg.data(), msg.size());

        if (this->verbose()) {
#if defined(TARGET_PICO)
            printf("sendMessage(0x%02x, 0x%02x, [%d bytes payload, %d total message size])\n", address, toInt(command), msg.size(), data.size());
#else
            this->log(std::format("sendMessage(0x{:02x}, 0x{:02x}, [{} bytes payload, {} total message size])", address, toInt(command), msg.size(), data.size()));
#endif
        }

        MsgHeader header{
            static_cast<uint8_t>(command),
            static_cast<uint8_t>(msg.size()),
            listenAddress(),
            computeChecksum(msg)
        };
        std::memcpy(data.data(), &header, MsgHeaderSize);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
        return i2cOut_.writeBytes(address, data);
#pragma GCC diagnostic pop
    }

};

} // namespace nl::rakis::raspberrypi::protocols