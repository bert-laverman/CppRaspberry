#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the I2C bus on the Raspberry Pi Zero 2 W

#include <string>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <i2c/smbus.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <interfaces/i2c.hpp>

namespace nl::rakis::raspberrypi::interfaces::zero2w {

    constexpr static const char *i2c1 = "/dev/i2c-1";
    constexpr static const char *i2c2 = "/dev/i2c-2";

    class Zero2WI2C : public virtual I2C {
        std::string interface_;
        int fd_;
        uint8_t address_{0};

        bool open()
        {
            if (fd_ >= 0) {
                return true;
            }

            if (verbose()) {
                std::cerr << "Opening " << interface_ << std::endl;
            }
            fd_ = ::open(interface_.c_str(), O_RDWR);
            if (fd_ < 0) {
                if (verbose()) {
                    std::cerr << "Failed to open " << interface_ << ". Errno=" << errno << std::endl;
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
                std::cerr << "Select device at 0x" << hex(address >> 4) << hex(address & 0x0f) << " for transfer." << std::endl;
            }
            if (::ioctl(fd_, I2C_SLAVE, address) < 0) {
                if (verbose()) {
                    std::cerr << "Failed to select device at 0x" << hex(address >> 4) << hex(address & 0x0f) << " for transfer. Errno=" << errno << std::endl;
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

        virtual void writeCmd(uint8_t address, uint8_t cmd) override {
            open();
            selectDevice(address);

            i2c_smbus_write_byte(fd_, cmd);
        }

        virtual void writeData(uint8_t address, uint8_t cmd, uint8_t data) override {
            open();
            selectDevice(address);

            i2c_smbus_write_byte_data(fd_, cmd, data);
        }

    };

} // namespace nl::rakis::raspberrypi::interfaces::zero2w