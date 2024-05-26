#pragma once
// Copyright 2024 by Bert Laverman, All rights reserved.


#include <cstddef>
#include <cstring>
#include <cstdint>

#include <map>
#include <span>
#include <vector>

#include <interfaces/i2c.hpp>
#include <protocols/protocol-driver.hpp>


namespace nl::rakis::raspberrypi::protocols {


/**
 * @brief This class manages communication through I2C, using one bus for incoming, and another for outgoing messages.
 */
template <typename I2COutImpl, typename I2CInImpl>
class I2CProtocolDriver : public ProtocolDriver {
    I2COutImpl& i2cOut_;
    I2CInImpl& i2cIn_;

    std::vector<CommandHandler> handlers_;

public:
    I2CProtocolDriver(I2COutImpl& out, I2CInImpl& in) : i2cOut_(out), i2cIn_(in), handlers_(32) {};

    virtual ~I2CProtocolDriver() {}

    I2CProtocolDriver(I2CProtocolDriver const &) = delete;
    I2CProtocolDriver(I2CProtocolDriver &&) = delete;
    I2CProtocolDriver &operator=(I2CProtocolDriver const &) = delete;
    I2CProtocolDriver &operator=(I2CProtocolDriver &&) = delete;

protected:
    inline I2CInImpl& i2cIn() { return i2cIn_; }
    inline I2COutImpl& i2cOut() { return i2cOut_; }
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

    inline bool haveHandler(Command command) const {
        unsigned index = toInt(command);
        return (handlers_.size() > index) && handlers_[index].handler;
    }
    inline const CommandHandler& handler(Command command) const {
        return handlers_.at(toInt(command));
    }

public:

    void registerHandler(Command command, std::string description, MsgCallback handler) override {
        unsigned index = toInt(command);
        if (index >= handlers_.size()) {
            handlers_.resize(index+1);
        }
        handlers_[index] = CommandHandler{ .command = command, .description = description, .handler = handler };
    }

    void enableControllerMode() override {
        i2cOut_.switchToControllerMode();
    }

    void disableControllerMode() override {
        i2cOut_.close();
    }

    virtual void enableResponderMode(uint8_t address) override {
        i2cIn_.switchToResponderMode(address, [this](Command command, uint8_t sender, const std::span<uint8_t> data) {
            unsigned index = toInt(command);
            if ((handlers_.size() > index) && handlers_[index].handler) {
                handlers_[index].handler(command, sender, data);
            } else {
                i2cIn_.log() << "No handler for command " << index << "\n";
            }
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

    bool sendMessage(uint8_t address, Command command, const std::span<uint8_t> msg) override {
        std::vector<uint8_t> data(MsgHeaderSize + msg.size(), 0x00);
        std::memcpy(data.data() + MsgHeaderSize, msg.data(), msg.size());

        MsgHeader header{
            static_cast<uint8_t>(command),
            static_cast<uint8_t>(msg.size()),
            i2cIn_.listenAddress(),
            computeChecksum(msg)
        };
        std::memcpy(data.data(), &header, MsgHeaderSize);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
        return i2cOut_.writeBytes(address, data);
#pragma GCC diagnostic pop
    }

    template <class Msg>
    inline bool sendMessage(uint8_t address, Command command, Msg& msg) {
        return sendMessage(address, command, std::span<uint8_t>(reinterpret_cast<uint8_t*>(&msg), sizeof(Msg)));
    }

    inline bool sendHello(uint8_t address, BoardId id)
    {
        return sendMessage(address, Command::Hello, std::span<uint8_t>(&((id.bytes)[0]), idSize));
    }

    inline bool sendHello(BoardId id)
    {
        return sendHello(0, id);
    }

    inline bool sendSetAddress(BoardId id, uint8_t address)
    {
        MsgSetAddress msg{ .boardId = id, .address = address };
        return sendMessage(0x00, Command::SetAddress, msg);
    }
};

} // namespace nl::rakis::raspberrypi::protocols