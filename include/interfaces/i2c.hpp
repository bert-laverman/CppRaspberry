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
        bool verbose_{ false };
        bool initialized_{ false };
        bool master_{ true };

        unsigned sdaPin_;
        unsigned sclPin_;

        uint8_t address_{ 0 };
        MsgCallback callback_;

    protected:
        inline void initialized(bool initialized) { initialized_ = initialized; }
        inline void controller(bool master) { master_ = master; }
        inline void sdaPin(unsigned sdaPin) { sdaPin_ = sdaPin; }
        inline void sclPin(unsigned sclPin) { sclPin_ = sclPin; }
        inline void listenAddress(uint8_t address) { address_ = address; }
        inline void callback(MsgCallback callback) { callback_ = callback; }

        virtual std::ostream &log() = 0;
        
    public:
        I2C() = default;
        I2C(unsigned sdaPin, unsigned sclPin) : sdaPin_(sdaPin), sclPin_(sclPin) {}

        I2C(I2C const &) = default;
        I2C(I2C &&) = default;
        I2C &operator=(I2C const &) = default;
        I2C &operator=(I2C &&) = default;
        ~I2C() = default;

        inline bool verbose() const { return verbose_; }
        inline void verbose(bool verbose) { verbose_ = verbose; }

        /**
         * @brief Check if the I2C bus has been initialized.
         */
        inline bool initialized() const { return initialized_; }

        /**
         * @brief Check if the I2C bus is in controller mode.
         */
        inline bool controller() const { return master_; }

        /**
         * @brief Check if the I2C bus is in responder mode.
         */
        inline bool responder() const { return !master_; }

        /**
         * @brief return the GPIO pin number used for the SDA line.
         */
        inline unsigned sdaPin() const { return sdaPin_; }

        /**
         * @brief return the GPIO pin number used for the SCL line.
         */
        inline unsigned sclPin() const { return sclPin_; }

        /**
         * @brief return the I2C address our Pi is using.
         */
        inline uint8_t listenAddress() const { return address_; }

        /**
         * @brief return the callback function used in responder mode.
         */
        inline MsgCallback callback() { return callback_; }

        /**
         * @brief Open the I2C bus for usage.
         */
        virtual void open() =0;

        /**
         * @brief Close the I2C bus, so it is back in an unintialized state.
         */
        virtual void close() =0;

        /**
         * @brief Cycle the I2C bus back to closed, then immediately open it again.
         */
        inline void reset() { close(); open(); }

        virtual void switchToControllerMode() =0;
        virtual void switchToResponderMode(uint8_t address, MsgCallback callback) =0;

        virtual bool readBytes(uint8_t address, std::span<uint8_t> data) =0;
        virtual bool writeBytes(uint8_t address, std::span<uint8_t> data) =0;
    };
}