#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the I2C bus on the Raspberry Pi Zero 2 W

#include <string>

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

    public:
        Zero2WI2C() = default;

        virtual ~Zero2WI2C() {
            if (fd_ >= 0) {
                ::close(fd_);
            }
        }

        void open()
        {
            fd_ = ::open(interface_.c_str(), O_RDWR);
        }

        virtual bool write(uint8_t address, [[maybe_unused]] std::span<uint8_t> const& value) override {
            if (fd_ < 0) {
                open();
            }
            if (ioctl(fd_, I2C_SLAVE, address) < 0) {
                return false;
            }
            return true;
        }
    };

} // namespace nl::rakis::raspberrypi::interfaces::zero2w