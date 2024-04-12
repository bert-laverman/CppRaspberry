#pragma once
// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47

#include <cstring>
#include <cstdint>

#include <vector>

#include <interfaces/i2c.hpp>
#include <protocols/i2c_protocol.hpp>


namespace nl::rakis::raspberrypi::protocols {

template <typename I2CImpl>
class I2CProtocolDriver {
    I2CImpl& i2c_;

    void messageReceived(uint8_t address, std::vector<uint8_t> const& data) 
    {
        i2c_.log() << "Not implemented.\n";
    }

public:
    I2CProtocolDriver(I2CImpl& i2c) : i2c_(i2c) {};

    virtual ~I2CProtocolDriver() = default;

    I2CProtocolDriver(I2CProtocolDriver const &) = delete;
    I2CProtocolDriver(I2CProtocolDriver &&) = delete;
    I2CProtocolDriver &operator=(I2CProtocolDriver const &) = delete;
    I2CProtocolDriver &operator=(I2CProtocolDriver &&) = delete;

    inline bool controller() const { return i2c_.master; }

    inline void reset() { i2c_.reset(); }
    inline void switchToMaster() { i2c_.switchToMaster(); }
    inline void switchToSlave(uint8_t address) {
        i2c_.switchToSlave(address, std::bind(&I2CProtocolDriver::messageReceived, this, std::placeholders::_1, std::placeholders::_2));
    }

    uint8_t computeChecksum(std::span<uint8_t> data) {
        uint8_t checksum = 0;
        for (auto byte : data) {
            checksum ^= byte;
        }
        return checksum;
    }

    bool sendHello(uint8_t address, uint8_t id[protocols::idSize])
    {
        std::vector<uint8_t> data(MsgHeaderSize + protocols::idSize);
        MsgHeader header{ static_cast<uint8_t>(Commands::Hello), protocols::idSize, i2c_.listenAddress(), computeChecksum(std::span(&id[0], protocols::idSize)) };
        std::memcpy(data.data(), &header, MsgHeaderSize);
        std::memcpy(data.data() + MsgHeaderSize, &id[0], protocols::idSize);

        return i2c_.writeBytes(address, data);
    }

    inline bool sendHello(uint8_t id[protocols::idSize])
    {
        return sendHello(0, id);
    }

    inline bool sendHello(uint8_t address, uint64_t boardId)
    {
        MsgHello hello{ boardId };

        return sendHello(address, hello.boardId.bytes);
    }

    inline bool sendHello(uint64_t boardId)
    {
        return sendHello(0, boardId);
    }

    void sendSetAddress(uint64_t boardId, uint8_t address);
};

} // namespace nl::rakis::raspberrypi::protocols