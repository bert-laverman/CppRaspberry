#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the I2C bus on the Raspberry Pi Zero 2 W

#include <string>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

extern "C" {
#include <i2c/smbus.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
}
#include <interfaces/i2c.hpp>

namespace nl::rakis::raspberrypi::interfaces::zero2w {

    constexpr static const char *i2c1 = "/dev/i2c-1";
    constexpr static const char *i2c2 = "/dev/i2c-2";

    class Zero2WI2C : public virtual I2C {
        std::string interface_;
        int fd_;
        uint8_t address_{0};

        inline static char hexHigh(uint8_t value) {
            return char(value >> 4 < 10 ? '0' + (value >> 4) : 'a' + (value >> 4) - 10);
        }
        inline static char hexLow(uint8_t value) {
            return char(value & 0x0f < 10 ? '0' + (value & 0x0f) : 'a' + (value & 0x0f) - 10);
        }
    protected:
        virtual std::ostream &log() {
            return std::cerr;
        }

    private:
        bool open()
        {
            if (fd_ >= 0) {
                return true;
            }

            if (verbose()) {
                log() << "Opening '" << interface_ << "'\n";
            }
            fd_ = ::open(interface_.c_str(), O_RDWR);
            if (fd_ < 0) {
                if (verbose()) {
                    log() << "Failed to open '" << interface_ << "'. Errno=" << errno << ".\n";
                }
                return false;
            }
            return true;
        }

        void close() {
            if (fd_ >= 0) {
                ::close(fd_);
                fd_ = -1;
            }
        }

        bool selectDevice(uint8_t address) {
            if (address_ == address) {
                return true;
            }
            if (verbose()) {
                log() << "Select device at 0x" << hexHigh(address) << hexLow(address) << " for transfer.\n";
            }
            if (::ioctl(fd_, I2C_SLAVE, address) < 0) {
                if (verbose()) {
                    log() << "Failed to select device at 0x" << hexHigh(address) << hexLow(address) << " for transfer. Errno=" << errno << ".\n";
                }
                return false;
            }
            return true;
        }

    public:
        Zero2WI2C() : interface_(i2c1), fd_(-1) {}

        virtual ~Zero2WI2C() {
            close();
        }

        virtual void writeByte(uint8_t address, uint8_t cmd) override {
            open();
            selectDevice(address);

            log() << std::format("Writing to 0x" << hexHigh(address) << hexLow(address) << ", byte 0x" << hexHigh(cmd) << hexLow(cmd) << ".\n";
            i2c_smbus_write_byte(fd_, cmd);
        }

        virtual void writeByteData(uint8_t address, uint8_t cmd, uint8_t data) override {
            open();
            selectDevice(address);

            log() << "Writing to 0x" << hexHigh(address) << hexLow(address)
                  << ", byte 0x" << hexHigh(cmd) << hexLow(cmd)
                  << " with data 0x" << hexHigh(data) << hexLow(data) << ".\n";
        }

    };

} // namespace nl::rakis::raspberrypi::interfaces::zero2w