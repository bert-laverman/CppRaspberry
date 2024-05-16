#pragma once
// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47

#include <cstddef>
#include <cstring>
#include <cstdint>

#include <span>
#include <vector>

#include <interfaces/i2c.hpp>
#include <protocols/i2c_protocol.hpp>


namespace nl::rakis::raspberrypi::protocols {

template <typename I2COutImpl, typename I2CInImpl>
class I2CProtocolDriver {
    I2COutImpl& i2cOut_;
    I2CInImpl& i2cIn_;
    bool dualMode_{ false };
    uint64_t seqNr_{ 0 };

public:
    I2CProtocolDriver(I2COutImpl& i2cOut, I2CInImpl& i2cIn) : i2cOut_(i2cOut), i2cIn_(i2cIn), dualMode_{ true } {};

    virtual ~I2CProtocolDriver() = default;

    I2CProtocolDriver(I2CProtocolDriver const &) = delete;
    I2CProtocolDriver(I2CProtocolDriver &&) = delete;
    I2CProtocolDriver &operator=(I2CProtocolDriver const &) = delete;
    I2CProtocolDriver &operator=(I2CProtocolDriver &&) = delete;

    inline uint64_t seqNr() const { return seqNr_; }

    /**
     * @brief Reset the I2C protocol driver. If operating in dual mode, set both in their correct modes.
     */
    void reset() {
        i2cOut_.reset();
        if (dualMode_) {
            i2cOut_.switchToControllerMode();
            i2cIn_.reset();
            i2cIn_.switchToResponderMode(i2cIn_.listenAddress(), i2cIn_.callback());
        }
    }

    inline void switchToControllerMode() { i2cOut_.switchToControllerMode(); }
    inline void switchToResponderMode(uint8_t address, interfaces::MsgCallback callback) {
        i2cIn_.switchToResponderMode(address, [this,callback](uint8_t command, uint8_t sender, const std::span<uint8_t> data) {
            seqNr_++;
            // i2c_.log() << "Message received, seqNr=" << seqNr_ << ".\n";
            callback(command, sender, data);
        });
    }

    uint8_t computeChecksum(std::span<uint8_t> data) {
        uint8_t checksum = 0;
        for (auto it = data.begin(); it != data.end(); ++it) {
            checksum ^= *it;
        }
        // std::cerr << std::format("Checksum for {} bytes of data is 0x{:02x}.\n", data.size(), checksum);
        return checksum;
    }

    bool sendMessage(uint8_t address, Commands command, const std::span<uint8_t> msg) {
        std::vector<uint8_t> data(MsgHeaderSize + msg.size());
        std::memcpy(data.data() + MsgHeaderSize, msg.data(), msg.size());

        MsgHeader header{
            static_cast<uint8_t>(command),
            static_cast<uint8_t>(msg.size()),
            i2cIn_.listenAddress(),
            computeChecksum(msg)
        };
        std::memcpy(data.data(), &header, MsgHeaderSize);

        return i2cOut_.writeBytes(address, data);
    }

    template <class Msg>
    inline bool sendMessage(uint8_t address, Commands command, Msg& msg) {
        return sendMessage(address, command, std::span<uint8_t>(reinterpret_cast<uint8_t*>(&msg), sizeof(Msg)));
    }

    inline bool sendHello(uint8_t address, uint8_t id[protocols::idSize])
    {
        return sendMessage(address, Commands::Hello, std::span<uint8_t>(id, protocols::idSize));
    }

    inline bool sendHello(uint8_t id[protocols::idSize])
    {
        return sendHello(0, id);
    }

    inline bool sendHello(uint8_t address, uint64_t boardId)
    {
        BoardId hello{ boardId };

        return sendHello(address, hello.bytes);
    }

    inline bool sendHello(uint64_t boardId)
    {
        return sendHello(0, boardId);
    }

    inline bool sendSetAddress(uint64_t boardId, uint8_t address)
    {
        MsgSetAddress msg{ .boardId={boardId}, .address=address };
        return sendMessage(0, Commands::SetAddress, msg);
    }
};

} // namespace nl::rakis::raspberrypi::protocols