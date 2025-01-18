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
#if !defined(TARGET_PICO)
#include <format>
#endif
#include <span>

#include <util/named-component.hpp>
#include <util/verbose-component.hpp>

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
constexpr unsigned MsgHeaderSize = sizeof(MsgHeader);

}

namespace nl::rakis::raspberrypi::interfaces
{

class I2C : public util::NamedComponent, public util::VerboseComponent {
    bool initialized_{ false };
    bool listening_{ false };

    unsigned sdaPin_;
    unsigned sclPin_;

    uint8_t address_{ 0 };
    protocols::MsgCallback callback_;

protected:

    /**
     * @brief Set the "initialized" flag.
     */
    void initialized(bool initialized) noexcept { initialized_ = initialized; }
    
    /**
     * @brief Check if the I2C bus has been initialized.
     */
    bool initialized() const noexcept { return initialized_; }

    /**
     * @brief Set if this interface is currently listening for incoming messages.
     */
    void listening(bool listening) { listening_ = listening; }

public:
    I2C() = default;

    I2C(I2C const &) = delete;
    I2C(I2C &&) = default;
    I2C &operator=(I2C const &) = delete;
    I2C &operator=(I2C &&) = default;

    virtual ~I2C() = default;

    /**
     * @brief Initialize the I2C bus for usage.
     */
    virtual void open() = 0;

    /**
     * @brief Close the I2C bus, so it is back in an unintialized state.
     */
    virtual void close() = 0;

    /**
     * @brief Cycle the I2C bus back to closed, then immediately open it again.
     */
    void reset() { close(); open(); }

    /**
     * @brief Find out if this implementation can listen for incoming messages.
     */
    virtual bool canListen() const noexcept = 0;

    /**
     * @brief Set the address to listen for. NOTE: If the value changes and it is currently listening, it will force a reset.
     */
    void listenAddress(uint8_t address) {
    #if defined(TARGET_PICO)
        printf("Setting listen address to 0x%02x\n", address);
    #else
        log(std::format("Setting listen address to 0x{:02x}", address));
    #endif
        if (address_ != address) {
            const bool needRestart = listening();
            if (needRestart) {
                stopListening();
            }
            address_ = address;
            if (needRestart) {
                startListening();
            }
        }
    }

    /**
     * @brief return the I2C address our Pi is using.
     */
    uint8_t listenAddress() const noexcept { return address_; }

    /**
     * @brief Start listening for incoming messages. If no address has been set, and the driver supports it, only Generall Call messages will be received.
     */
    virtual void startListening() = 0;

    /**
     * @brief Stop listening for incoming messages.
     */
    virtual void stopListening() = 0;

    /**
     * @brief Check if we are currently listening for incoming messages.
     */
    bool listening() const noexcept { return listening_; }

    /**
     * @brief Find out if this implementation can send messages.
     */
    virtual bool canSend() const noexcept = 0;

    /**
     * @brief Set the GPIO pin used for data. NOTE: If the value changes, the connection will be forcibly closed.
     */
    void sdaPin(unsigned pin) {
        if (sdaPin_ != pin) {
            close();
            sdaPin_ = pin;
        }
    }

    /**
     * @brief return the GPIO pin number used for the SDA line.
     */
    unsigned sdaPin() const noexcept { return sdaPin_; }

    /**
     * @brief Set the GPIO pin used the clock. NOTE: If the value changes, the connection will be forcibly closed.
     */
    void sclPin(unsigned pin) {
        if (sclPin_ != pin) {
            close();
            sclPin_ = pin;
        }
    }

    /**
     * @brief return the GPIO pin number used for the SCL line.
     */
    unsigned sclPin() const { return sclPin_; }

    /**
     * @brief Set the callback to be used for incoming messages.
     */
    void callback(protocols::MsgCallback callback) noexcept { callback_ = callback; }

    /**
     * @brief return the callback function used in responder mode.
     */
    protocols::MsgCallback callback() const noexcept { return callback_; }

    /**
     * @brief Attempt to send a span of bytes to a listener at the given address.
     *
     * @return true if successfull, which means there was a listener active at that address.
     */
    virtual bool write(uint8_t address, std::span<uint8_t> data) = 0;

};

} // namespace nl::rakis::raspberrypi::interfaces