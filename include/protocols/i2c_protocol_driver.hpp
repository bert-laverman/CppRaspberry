#pragma once
// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47

#include <cstdint>

#include <interfaces/i2c.hpp>

#include <protocols/i2c_protocol.hpp>


namespace nl::rakis::raspberrypi::protocols {

template <typename I2CImpl>
class I2CProtocolDriver {
    I2CImpl& i2c_;

    void messageReceived(uint8_t address, std::vector<uint8_t> const& data);

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

    void sendHello(uint64_t boardId);
    void sendSetAddress(uint64_t boardId, uint8_t address);
};

} // namespace nl::rakis::raspberrypi::protocols