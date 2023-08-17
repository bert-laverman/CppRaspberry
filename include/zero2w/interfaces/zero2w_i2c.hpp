#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the I2C bus on the Raspberry Pi Zero 2 W

#include <string>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <interfaces/i2c.hpp>

namespace nl::rakis::raspberrypi::interfaces::zero2w {

    constexpr static const char *i2c1 = "/dev/i2c-1";
    constexpr static const char *i2c2 = "/dev/i2c-2";

    class Zero2WI2C : public virtual I2C {
        std::string interface_;
        int fd_;

        void open()
        {
            if (fd_ >= 0) {
                return;
            }
            if (verbose()) {
                std::cerr << "Opening " << interface_ << std::endl;
            }
            fd_ = ::open(interface_.c_str(), O_RDWR);
        }

        void writeCmd(uint8_t address, uint8_t cmd) {

        }

    public:
        Zero2WI2C() : interface_(i2c1), fd_(-1) {}

        virtual ~Zero2WI2C() {
            if (fd_ >= 0) {
                ::close(fd_);
            }
        }

        virtual bool write(uint8_t address, const uint8_t* buf, unsigned len) override {
            if (fd_ < 0) {
                open();
            }
            if (verbose()) {
                std::cerr << "Trying to select device at 0x" << hex(address >> 4) << hex(address & 0x0f) << " for transfer." << std::endl;
            }
            if (::ioctl(fd_, I2C_SLAVE, address) < 0) {
                if (verbose()) {
                    std::cerr << "Failed to select device at 0x" << hex(address >> 4) << hex(address & 0x0f) << " for transfer. Errno=" << errno << std::endl;
                }
                return false;
            }
            if (verbose()) {
                std::cerr << "Sending " << len << " byte(s)." << std::endl;
            }
            ::write(fd_, buf, len);

            return true;
        }
    };

} // namespace nl::rakis::raspberrypi::interfaces::zero2w