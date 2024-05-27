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

#include <span>
#include <string>

#include <protocols/messages.hpp>


namespace nl::rakis::raspberrypi::protocols {


/**
 * @brief A handler for a particular command.
 */
struct CommandHandler {
    Command command;
    std::string description;
    MsgCallback handler;
};

/**
 * @brief A ProtocolDriver allows you to send and receive messages using the message definitions in this namespace.
 */
class ProtocolDriver
{
public:

    /**
     * @brief Register a handler for a certain command.
     */
    virtual void registerHandler(Command command, std::string description, MsgCallback handler) = 0;

    /**
     * @brief Enable controller mode, allowing you to send messages. By default this mode is enabled at startup. It depends on
     *        the protocol driver if this mode can be enabled at the same time as reponder mode.
     */
    virtual void enableControllerMode() = 0;

    /**
     * @brief Disable controller mode.
     */
    virtual void disableControllerMode() = 0;

    inline static constexpr uint8_t GeneralCallAddress = 0x00;

    /**
     * @brief Enable responder mode, which allows this Raspberry Pi to receive messages. While enabled, General Call messages
     *        will always be picked up if the hardware supports it. (NOTE the pigpiod daemon cannot do this)
     *
     * @param address The address this Raspberry Pi will respond to, by default only the General Call address.
     */
    virtual void enableResponderMode(uint8_t address =GeneralCallAddress) = 0;

    /**
     * @brief Check if the Raspberry Pi has responder mode enabled and is listening.
     */
    virtual bool isListening() const = 0;

    /**
     * @brief Return the address the Raspberry Pi is listening for.
     */
    virtual uint8_t listenAddress() const = 0;

    /**
     * @brief Stop listening for messages.
     */
    virtual void disableResponderMode() = 0;

    /**
     * @brief Send a message.
     */
    virtual bool sendMessage(uint8_t address, Command command, const std::span<uint8_t> body) = 0;
};

} // namespace nl::rakis::raspberrypi::protocols