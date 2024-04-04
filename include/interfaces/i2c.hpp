#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the I2C bus

#include <cstdint>

#include <iostream>
#include <functional>
#include <vector>
#include <span>


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
    inline constexpr unsigned MsgHeaderSize = sizeof(MsgHeader);

}

namespace nl::rakis::raspberrypi::interfaces
{

    using MsgCallback = std::function<void(uint8_t command, uint8_t sender, const std::vector<uint8_t>& data)>;

    class I2C {
        bool verbose_{false};
        bool master_{true};

        uint8_t address_{0};
        MsgCallback callback_;

    protected:
        I2C() = default;
        I2C(I2C const &) = default;
        I2C(I2C &&) = default;
        I2C &operator=(I2C const &) = default;
        I2C &operator=(I2C &&) = default;

        virtual std::ostream &log() = 0;
        
    public:
        virtual ~I2C() = default;

        inline bool verbose() const { return verbose_; }
        inline void verbose(bool verbose) { verbose_ = verbose; }

        inline bool controller() const { return master_; }
        inline void controller(bool master) { master_ = master; }
        inline bool responder() const { return !master_; }

        inline uint8_t listenAddress() const { return address_; }
        inline void listenAddress(uint8_t address) { address_ = address; }
        inline MsgCallback callback() { return callback_; }
        inline void callback(MsgCallback callback) { callback_ = callback; }

        virtual void reset() =0;
        virtual void switchToControllerMode() =0;
        virtual void switchToResponderMode(uint8_t address, MsgCallback callback) =0;

        virtual void writeByte(uint8_t address, uint8_t value) =0;
        virtual void writeBytes(uint8_t address, std::span<uint8_t> data) =0;

        virtual void writeByteData(uint8_t address, uint8_t value, uint8_t data) =0;
    };
}