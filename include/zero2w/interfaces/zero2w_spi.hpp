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

namespace nl::rakis::raspberrypi::interfaces::zero2w
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
        int fd_;

    protected:
        virtual std::ostream &log() {
            return std::cerr;
        }

        void writeBlocking(std::function<std::array<uint8_t, 2>(unsigned)> const &value)
        {
            if (fd_ < 0) {
                open();
            }
            const size_t bufSize{1 + numModules() * 2};
            std::vector<uint8_t> tx_buf;
            std::vector<uint8_t> rx_buf;

            tx_buf.resize(bufSize);
            tx_buf.assign(bufSize, 0);
            rx_buf.resize(bufSize);
            rx_buf.assign(bufSize, 0);

            tx_buf[0] = 0x40;
            for (unsigned i = 0; i < numModules(); ++i)
            {
                auto const &bytes = value(i);
                tx_buf[1 + i * 2] = bytes[0];
                tx_buf[2 + i * 2] = bytes[1];
            }

            if (verbose()) {
                log() << "Writing ";
                for (auto const &byte : tx_buf)
                    log()  << hex(byte >> 4) << hex(byte & 0x0f) << " ";
                log() << std::endl;
            }
            struct spi_ioc_transfer tr{
                .tx_buf = (unsigned long)(tx_buf.data()),
                .rx_buf = (unsigned long)(rx_buf.data()),
                .len = bufSize,
                .speed_hz = 5000000,
                .delay_usecs = 0,
                .bits_per_word = 8,
                .cs_change = 0,
                .tx_nbits = 0,
                .rx_nbits = 0,
                .word_delay_usecs= 0,
                .pad = 0,
                };
            ioctl(fd_, SPI_IOC_MESSAGE(1), &tr);
        }

    public:
        Zero2WSPI(const char *interface)
            : SPI(), interface_(interface), fd_(-1)
        {
        }

        Zero2WSPI() : Zero2WSPI(spi0)
        {
        }

        virtual ~Zero2WSPI()
        {
            ::close(fd_);
        }

        void open()
        {
            fd_ = ::open(interface_.c_str(), O_RDWR);
        }

        virtual void select()
        {
        }

        virtual void deselect()
        {
        }

        virtual void writeAll(std::array<uint8_t, 2> const &value)
        {
            writeBlocking([value](unsigned) { return value; });
        }

        virtual void writeAll(std::function<std::array<uint8_t, 2>(unsigned)> const &value)
        {
            writeBlocking(value);
        }
    };

} // namespace nl::rakis::raspberrypi::interfaces::zero2w