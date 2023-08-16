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

        virtual void writeBlocking(std::array<uint8_t, 2> const &value)
        {
            uint8_t tx_buf[3];
            uint8_t rx_buf[3];

            std::memset(tx_buf, 0, sizeof(tx_buf));
            std::memset(rx_buf, 0, sizeof(rx_buf));

            tx_buf[0] = 0x40;
            tx_buf[1] = value[0];
            tx_buf[2] = value[1];

            std::cerr << "Writing "
                      << char('0' + (tx_buf[0] >> 4)) << char('0' + (tx_buf[0] & 0x0f)) << ", "
                      << char('0' + (tx_buf[1] >> 4)) << char('0' + (tx_buf[1] & 0x0f)) << ", "
                      << char('0' + (tx_buf[2] >> 4)) << char('0' + (tx_buf[2] & 0x0f)) << ", "
                      << std::endl;

            struct spi_ioc_transfer tr{
                .tx_buf = (unsigned long)tx_buf,
                .rx_buf = (unsigned long)rx_buf,
                .len = 3,
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

    };

} // namespace nl::rakis::raspberry::interfaces::zero2w