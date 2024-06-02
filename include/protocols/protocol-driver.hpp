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
#include <vector>

#include <util/verbose-component.hpp>
#include <util/message-queue.hpp>
#include <protocols/messages.hpp>


namespace nl::rakis::raspberrypi::protocols {


/**
 * @brief A handler for a particular command.
 */
struct CommandHandler {
    Command command;
    std::string description;
    util::MessageQueue::Handler handler;
};

/**
    * @brief The address used for broadcast messages, called "General Call" for I2C.
    */
inline static constexpr uint8_t GeneralCallAddress = 0x00;

/**
 * @brief A ProtocolDriver allows you to send and receive messages using the message definitions in this namespace.
 */
 template <typename QueueImpl>
class ProtocolDriver : public util::VerboseComponent
{
    QueueImpl incoming_;
    QueueImpl outgoing_;

    std::vector<CommandHandler> handlers_;

protected:

    static void noopHandler(Command command, uint8_t sender, [[maybe_unused]] const std::vector<uint8_t>& data){
        log() << "No registered handler for command " << toInt(command) << " from " << sender << ". Ignoring message.\n";
    }

    inline bool haveHandler(Command command) const {
        unsigned index = toInt(command);
        return (handlers_.size() > index) && handlers_[index].handler;
    }

    inline util::MessageQueue::Handler handler(Command command) const {
        return haveHandler(command) ? handlers_.at(toInt(command)).handler : noopHandler;
    }

public:
    ProtocolDriver() = default;
    ProtocolDriver(const ProtocolDriver&) = default;
    ProtocolDriver(ProtocolDriver&&) = default;
    ~ProtocolDriver() = default;

    ProtocolDriver& operator=(const ProtocolDriver&) = default;
    ProtocolDriver& operator=(ProtocolDriver&&) = default;


    /**
     * @brief Register a handler for a certain command.
     *
     * @param command     The command to handle.
     * @param description A description of the handler.
     * @param handler     The actual handler.
     */
    void registerHandler(Command command, std::string description, util::MessageQueue::Handler handler) {
        unsigned index = toInt(command);
        if (index >= handlers_.size()) {
            handlers_.resize(index+1);
        }
        handlers_[index] = CommandHandler{ .command = command, .description = description, .handler = handler };
    }


    /**
     * @brief try to call the appropriate handler for a message.
     */
    inline void handle(Command command, uint8_t sender, const std::vector<uint8_t>& data) {
        handler(command) (command, sender, data);
    }


    /**
     * @brief Enable controller mode, allowing you to send messages. By default this mode is enabled at startup. It depends on
     *        the protocol driver if this mode can be enabled at the same time as reponder mode.
     */
    virtual void enableControllerMode() = 0;

    /**
     * @brief Disable controller mode.
     */
    virtual void disableControllerMode() = 0;

    /**
     * @brief Enable responder mode, which allows this Raspberry Pi to receive messages. While enabled, General Call messages
     *        will always be picked up if the hardware supports it. (NOTE the pigpiod daemon cannot do this)
     *
     * @param address The address this Raspberry Pi will respond to, by default only the General Call address.
     */
    virtual void enableResponderMode(uint8_t address = GeneralCallAddress) = 0;

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
    virtual bool sendMessage(Command command, uint8_t address, const std::span<uint8_t> body) = 0;

    template <class Msg>
    inline bool sendMessage(Command command, uint8_t address, Msg& msg) {
        return sendMessage(command, address, std::span<uint8_t>(reinterpret_cast<uint8_t*>(&msg), sizeof(Msg)));
    }


    inline bool haveIncoming() { return incoming_.haveMessages(); }
    inline void pushIncoming(protocols::Command command, uint8_t address, std::span<uint8_t> data) {
        incoming_.push(command, address, data);
    }
    inline void processIncoming() {
        incoming_.processAll([this](Command command, uint8_t address, const std::vector<uint8_t>& data) {
            handle(command, address, data);
        });
    }


    inline bool haveOutgoing() const { return outgoing_.haveMessages(); }
    inline void pushOutgoing(protocols::Command command, uint8_t address, std::span<uint8_t> data) {
        outgoing_.push(command, address, data);
    }
    inline void processOutgoing() {
        outgoing_.processAll([this](Command command, uint8_t address, const std::vector<uint8_t>& data) {
            if (!sendMessage(command, address, data) && verbose()) {
                log() << "Failed to send " << toInt(command) << " to " << address << ", no response.\n";
            }
        });
    }
};

} // namespace nl::rakis::raspberrypi::protocols