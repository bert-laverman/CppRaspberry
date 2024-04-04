#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the I2C bus on the Raspberry Pi Zero 2 W

#include <string>
#include <format>
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
                log() << std::format("Opening '{}'\n", interface_);
            }
            fd_ = ::open(interface_.c_str(), O_RDWR);
            if (fd_ < 0) {
                if (verbose()) {
                    log() << std::format("Failed to open '{}'. Errno={}.\n", interface_, errno);
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
                log() << std::format("Select device at 0x{:02x} for transfer.\n", address);
            }
            if (::ioctl(fd_, I2C_SLAVE, address) < 0) {
                if (verbose()) {
                    log() << std::format("Failed to select device at 0x{:02x} for transfer. Errno={}.\n", address, errno);
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

            log() << std::format("Writing to 0x{:02x}, byte 0x{:02x}.\n", address, cmd);
            i2c_smbus_write_byte(fd_, cmd);
        }

        virtual void writeByteData(uint8_t address, uint8_t cmd, uint8_t data) override {
            open();
            selectDevice(address);

            log() << std::format("Writing to 0x{:02x}, byte 0x{:02x} with data 0x{:02x}.\n", address, cmd, data);
        }

    };

} // namespace nl::rakis::raspberrypi::interfaces::zero2w