#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the I2C bus on the Raspberry Pi Zero 2 W

#include <string>
#include <iostream>

#include <interfaces/i2c.hpp>

namespace nl::rakis::raspberrypi::interfaces {

    constexpr static const char *i2c1 = "/dev/i2c-1";
    constexpr static const char *i2c2 = "/dev/i2c-2";

    class Zero2WI2C : public virtual I2C {
        std::string interface_;
        int fd_{ -1 };
        uint8_t address_{ 0 };


        inline static char hexHigh(uint8_t value) {
            return char(((value >> 4) < 10) ? ('0' + (value >> 4)) : ('a' + (value >> 4) - 10));
        }
        inline static char hexLow(uint8_t value) {
            return char(((value & 0x0f) < 10) ? '0' + (value & 0x0f) : ('a' + (value & 0x0f) - 10));
        }

    protected:
        virtual std::ostream &log() {
            return std::cerr;
        }

        bool readOneByte(uint8_t &value);

    public:
        Zero2WI2C() : interface_(i2c1) {}
        Zero2WI2C(const char *interface) : interface_(interface) {}

        virtual ~Zero2WI2C();

        virtual void open() override;
        virtual void close() override;

        virtual void switchToControllerMode() override;

        virtual void switchToResponderMode(uint8_t address, MsgCallback cb) override;

        virtual bool readBytes(uint8_t address, std::span<uint8_t> data) override;
        virtual bool writeBytes(uint8_t address, std::span<uint8_t> data) override;
        bool readMessage(protocols::MsgHeader &header, std::vector<uint8_t> &data);
    };

} // namespace nl::rakis::raspberrypi::interfaces::zero2w