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
static constexpr uint8_t GeneralCallAddress = 0x00;

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

    static void noopHandler(Command command, uint8_t sender, [[maybe_unused]] const std::vector<uint8_t>& data) {
        log() << "No registered handler for command " << toInt(command) << " from " << sender << ". Ignoring message.\n";
    }

    bool haveHandler(Command command) const {
        unsigned index = toInt(command);
        return (handlers_.size() > index) && handlers_[index].handler;
    }

    util::MessageQueue::Handler handler(Command command) const {
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
    void handle(Command command, uint8_t sender, const std::vector<uint8_t>& data) {
        handler(command) (command, sender, data);
    }

    /**
     * @brief Initialize the protocol driver for usage.
     */
    virtual void open() = 0;

    /**
     * @brief Close the protocol driver, so it is back in an unintialized state.
     */
    virtual void close() = 0;

    /**
     * @brief Cycle the protocol driver back to closed, then immediately open it again.
     */
    void reset() { close(); open(); }

    /**
     * @brief Find out if we can listen for incoming messages.
     */
    virtual bool canListen() const noexcept = 0;

    /**
     * @brief Set the address to listen to.
     *
     * @param address The address to listen to.
     */
    virtual void listenAddress(uint8_t address) = 0;

    /**
     * @brief Get the address we are listening to.
     *
     * @return The address we are listening to, or 0 if not listening.
     */
    virtual uint8_t listenAddress() const = 0;

    /**
     * @brief Start listening for incoming messages.
     */
    virtual void startListening() = 0;

    /**
     * @brief Stop listening for incoming messages.
     */
    virtual void stopListening() = 0;

    /**
     * @brief Check if we are currently listening for incoming messages.
     */
    virtual bool listening() const noexcept = 0;

    /**
     * @brief Find out if this implementation can send messages.
     */
    virtual bool canSend() const noexcept = 0;

    /**
     * @brief Send a message.
     *
     * @param command The command to send.
     * @param address The address to send it to.
     * @param body    The payload of the message.
     */
    virtual bool sendMessage(Command command, uint8_t address, const std::span<uint8_t> body) = 0;

    /**
     * @brief Send a message.
     *
     * @param command The command to send.
     * @param address The address to send it to.
     * @param msg     The message to send.
     */
    template <class Msg>
    bool sendMessage(Command command, uint8_t address, Msg& msg) {
        return sendMessage(command, address, std::span<uint8_t>(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&msg)), sizeof(Msg)));
    }

    /**
     * @brief Send a message.
     *
     * @param command The command to send.
     * @param address The address to send it to.
     * @param payload The payload of the message.
     */
    bool sendMessage(Command command, uint8_t address, const std::vector<uint8_t>& payload) {
        return sendMessage(command, address, std::span<uint8_t>(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(payload.data())), payload.size()));
    }

    /**
     * @brief Check if there are incoming messages in the queue.
      */
    bool haveIncoming() { return incoming_.haveMessages(); }

    /**
     * @brief Add a message to the incoming queue.
     *
     * @param command The command of the message.
     * @param address The address of the sender.
     * @param data    The payload of the message.
     */
    void pushIncoming(protocols::Command command, uint8_t address, const std::span<uint8_t> data) {
        incoming_.push(command, address, data);
    }

    /**
     * @brief Process all messagesin the incoming queue.
     */
    void processIncoming() {
        incoming_.processAll([this](Command command, uint8_t address, const std::vector<uint8_t>& data) {
            handle(command, address, data);
        });
    }

    /**
      * @brief Check if there are messages in the outgoing queue.
      */
    bool haveOutgoing() const { return outgoing_.haveMessages(); }

    /**
     * @brief Add a message to the outgoing queue.
     *
     * @param command The command of the message.
     * @param address The address of the recipient.
     * @param data    The payload of the message.
     */
    void pushOutgoing(protocols::Command command, uint8_t address, std::span<uint8_t> data) {
        outgoing_.push(command, address, data);
    }

    /**
     * @brief Process all messages in the outgoing queue.
     */
    void processOutgoing() {
        outgoing_.processAll([this](Command command, uint8_t address, const std::vector<uint8_t>& data) {
            if (!sendMessage(command, address, data) && verbose()) {
                log() << "Failed to send " << static_cast<int>(toInt(command)) << " to " << static_cast<int>(address) << ", no response.\n";
            }
        });
    }
};

} // namespace nl::rakis::raspberrypi::protocols