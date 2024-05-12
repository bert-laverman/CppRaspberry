#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: Provide an interface to the SPI bus

#include <array>
#include <functional>
#include <string>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include <interfaces/spi.hpp>

namespace nl::rakis::raspberrypi::interfaces
{

    constexpr static const char *spi0{"/dev/spidev0.0"};
    constexpr static const char *spi1{"/dev/spidev0.1"};

    inline char hex(int value)
    {
        return char(value < 10 ? '0' + value : 'a' + value - 10);
    }

    class Zero2WSPI : public virtual SPI
    {
        std::string interface_;
        int fd_{ -1 };
        int channel_{ -1 };

    protected:
        virtual std::ostream &log() override {
            return std::cerr;
        }

        [[maybe_unused]]
        void writeBlocking_spidev(std::function<std::array<uint8_t, 2>(unsigned)> const &value);

        [[maybe_unused]]
        void writeBlocking_pigpiod(std::function<std::array<uint8_t, 2>(unsigned)> const &value);

    public:
        Zero2WSPI(const char *interface)
            : SPI(), interface_(interface)
        {
        }

        Zero2WSPI() : Zero2WSPI(spi0)
        {
        }

        Zero2WSPI(Zero2WSPI const &) = default;
        Zero2WSPI(Zero2WSPI &&) = default;
        Zero2WSPI &operator=(Zero2WSPI const &) = default;
        Zero2WSPI &operator=(Zero2WSPI &&) = default;

        virtual ~Zero2WSPI()
        {
            close();
        }

        virtual void open() override;

        virtual void close() override;

        virtual void select() override;

        virtual void deselect() override;

        virtual void writeAll(std::array<uint8_t, 2> const &value) override;

        virtual void writeAll(std::function<std::array<uint8_t, 2>(unsigned)> const &value) override;

    };

} // namespace nl::rakis::raspberrypi::interfaces::zero2w