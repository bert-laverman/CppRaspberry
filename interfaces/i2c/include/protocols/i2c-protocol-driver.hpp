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
#include <memory>

#if !defined(TARGET_PICO)
#include <format>
#endif

#include <interfaces/i2c.hpp>
#include <protocols/protocol-driver.hpp>


namespace nl::rakis::raspberrypi::protocols {


/**
 * @brief This class manages communication through I2C, using one bus for incoming, and another for outgoing messages.
 */
template <typename QueueImpl>
class I2CProtocolDriver : public ProtocolDriver<QueueImpl> {
    std::shared_ptr<interfaces::I2C> i2cOut_;
    std::shared_ptr<interfaces::I2C> i2cIn_;

protected:
    /**
     * @brief Reset the I2C protocol driver. If operating in dual mode, set both in their correct modes.
     */
    void reset() {
        if (i2cOut_) {
            i2cOut_->reset();
        }
        if (i2cIn_) {
            auto listening = i2cIn_->listening();
            i2cIn_->stopListening();
            i2cIn_->reset();
            if (listening) {
                i2cIn_->startListening();
            }
        }
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
    I2CProtocolDriver() = default;

    virtual ~I2CProtocolDriver() {}

    I2CProtocolDriver(I2CProtocolDriver const &) = delete;
    I2CProtocolDriver(I2CProtocolDriver &&) = delete;
    I2CProtocolDriver &operator=(I2CProtocolDriver const &) = delete;
    I2CProtocolDriver &operator=(I2CProtocolDriver &&) = delete;

    I2CProtocolDriver(std::shared_ptr<interfaces::I2C> out, std::shared_ptr<interfaces::I2C> in) : i2cOut_(out), i2cIn_(in) {};

    void addInterface(std::shared_ptr<interfaces::I2C> i2c) {
        if (i2c->canListen()) {
            i2cIn_ = i2c;
        }
        if (i2c->canSend()) {
            i2cOut_ = i2c;
        }
    }

    void i2cIn(std::shared_ptr<interfaces::I2C> i2c) { i2cIn_ = i2c; }
    std::weak_ptr<interfaces::I2C> i2cIn() const { return i2cIn_; }
    void i2cOut(std::shared_ptr<interfaces::I2C> i2c) { i2cOut_ = i2c; }
    std::weak_ptr<interfaces::I2C> i2cOut() const { return i2cOut_; }

    /**
     * @brief Initialize the protocol driver for usage.
     */
    virtual void open() override {
        if (i2cIn_) {
            i2cIn_->open();
        }
        if (i2cOut_) {
            i2cOut_->open();
        }
    }

    /**
     * @brief Close the protocol driver, so it is back in an unintialized state.
     */
    virtual void close() override {
        if (i2cIn_) {
            i2cIn_->close();
        }
        if (i2cOut_) {
            i2cOut_->close();
        }
    }

    /**
     * @brief Find out if we can listen for incoming messages.
     */
    virtual bool canListen() const noexcept override {
        return i2cIn_ && i2cIn_->canListen();
    }

    /**
     * @brief Set the address to listen to.
     *
     * @param address The address to listen to.
     */
    virtual void listenAddress(uint8_t address) override {
        if (i2cIn_) {
            i2cIn_->listenAddress(address);
        } else {
            this->log("No incoming I2C interface available, cannot set listen address.");
        }
    }

    /**
     * @brief Get the address we are listening to.
     *
     * @return The address we are listening to, or 0 if not listening.
     */
    virtual uint8_t listenAddress() const override {
        return i2cIn_ ? i2cIn_->listenAddress() : 0;
    }

    /**
     * @brief Start listening for incoming messages.
     */
    virtual void startListening() override {
        if (i2cIn_) {
            i2cIn_->callback([this](Command command, uint8_t sender, const std::span<uint8_t> data) {
                this->pushIncoming(command, sender, data);
            });
            i2cIn_->startListening();
        } else {
            this->log("No incoming I2C interface available, cannot start listening.");
        }
    }

    /**
     * @brief Stop listening for incoming messages.
     */
    virtual void stopListening() override {
        if (i2cIn_) {
            i2cIn_->stopListening();
        } else {
            this->log("No incoming I2C interface available, cannot stop listening.");
        }
    }

    /**
     * @brief Check if we are currently listening for incoming messages.
     */
    virtual bool listening() const noexcept override {
        return i2cIn_ && i2cIn_->listening();
    }

    /**
     * @brief Find out if this implementation can send messages.
     */
    virtual bool canSend() const noexcept override {
        return i2cOut_ && i2cOut_->canSend();
    }

    /**
     * @brief Send a message.
     *
     * @param command The command to send.
     * @param address The address to send it to.
     * @param msg     The payload of the message.
     */
    virtual bool sendMessage(Command command, uint8_t address, const std::span<uint8_t> msg) override {
        if (!i2cOut_) {
            this->log("No outgoing I2C interface available, cannot send message.");
            return false;
        }
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
        return i2cOut_->write(address, data);
#pragma GCC diagnostic pop
    }

};

} // namespace nl::rakis::raspberrypi::protocols