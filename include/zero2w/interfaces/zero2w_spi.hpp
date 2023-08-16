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

#include "../../interfaces/spi.hpp"

namespace nl::rakis::raspberry::interfaces::zero2w
{

    constexpr static const char *spi0{"/dev/spidev0.0"};
    constexpr static const char *spi1{"/dev/spidev0.1"};

    enum class DefaultPin : uint
    {
        SPI0_MISO = 9,
        SPI0_CS = 8,
        SPI0_SCK = 11,
        SPI0_MOSI = 10
    };

    inline consteval uint operator+(DefaultPin pin)
    {
        return uint(pin);
    };

    inline char hex(int value)
    {
        return char(value < 10 ? '0' + value : 'a' + value - 10);
    }

    class Zero2WSPI : public virtual SPI
    {
        std::string interface_;
        int fd_;

        uint csPin_;
        uint sckPin_;
        uint mosiPin_;
        uint misoPin_;

    protected:
        unsigned long bytes2buf(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) const
        {
            union {
                uint8_t bytes[4];
                unsigned long buf;
            } u{.bytes = {byte0, byte1, byte2, byte3}};
            return u.buf;
        }

        void writeBlocking(std::function<std::array<uint8_t, 2>(uint)> const &value)
        {
            const size_t bufSize{1 + numModules() * 2};
            std::vector<uint8_t> tx_buf;
            std::vector<uint8_t> rx_buf;

            tx_buf.resize(bufSize);
            tx_buf.assign(bufSize, 0);
            rx_buf.resize(bufSize);
            rx_buf.assign(bufSize, 0);

            tx_buf[0] = 0x40;
            for (uint i = 0; i < numModules(); ++i)
            {
                auto const &bytes = value(i);
                tx_buf[1 + i * 2] = bytes[0];
                tx_buf[2 + i * 2] = bytes[1];
            }

            std::cerr << "Writing ";
            for (auto const &byte : tx_buf)
                std::cerr  << hex(byte >> 4) << hex(byte & 0x0f) << " ";
            std::cerr << std::endl;

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
                .pad = 0,
                };
            ioctl(fd_, SPI_IOC_MESSAGE(1), &tr);
        }

    public:
        Zero2WSPI(const char *interface, uint csPin, uint sckPin, uint mosiPin, uint misoPin)
            : interface_(interface), SPI(csPin, sckPin, mosiPin, misoPin)
        {
        }

        Zero2WSPI(uint csPin, uint sckPin, uint mosiPin, uint misoPin) : Zero2WSPI(spi0, csPin, sckPin, mosiPin, misoPin)
        {
        }

        Zero2WSPI() : Zero2WSPI(+DefaultPin::SPI0_CS, +DefaultPin::SPI0_SCK, +DefaultPin::SPI0_MOSI, +DefaultPin::SPI0_MISO)
        {
        }

        virtual ~Zero2WSPI()
        {
            ::close(fd_);
        }

        virtual void open()
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
            writeBlocking([value](uint) { return value; });
        }

        virtual void writeAll(std::function<std::array<uint8_t, 2>(uint)> const &value)
        {
            writeBlocking(value);
        }
    };

} // namespace nl::rakis::raspberry::interfaces::zero2w