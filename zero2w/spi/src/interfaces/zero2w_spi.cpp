// Copyright (c) 2024 by Bert Laverman, All rights reserved

#include <stdexcept>
#include <chrono>
#include <thread>

extern "C" {
#include <pigpiod_if2.h>
}

#include <interfaces/zero2w_spi.hpp>

using namespace nl::rakis::raspberrypi::interfaces;

//#define SPI_SPIDEV
#define SPI_PIGPIO


void Zero2WSPI::open()
{
#if defined(SPI_SPIDEV)
    if (fd_ < 0) {
        if (verbose()) {
            log() << "Opening SPI interface: " << interface_ << std::endl;
        }
        fd_ = ::open(interface_.c_str(), O_RDWR);
        if (fd_ < 0) {
            log() << "Failed to open SPI interface: " << strerror(errno) << std::endl;
            throw std::runtime_error("Failed to open SPI interface");
        }
    }
#endif
#if defined(SPI_PIGPIO)
    if (channel_ < 0) {
        if (verbose()) {
            log() << "Opening a channel to pigpiod.\n";
        }
        channel_ =  pigpio_start(nullptr, nullptr);
        if (channel_ < 0) {
            log() << "Failed to open a channel to pigpiod.\n";
            throw std::runtime_error("Failed to open a channel to pigpiod.");
        }
    }
    if (fd_ < 0) {
        if (verbose()) {
            log() << "Opening SPI channel 0.\n";
        }
        fd_ =  spi_open(channel_, 0, 10*1000*1000, 0x0000);
        if (fd_ < 0) {
            switch (fd_) {
            case PI_BAD_SPI_CHANNEL:
                log() << "Bad SPI channel #.\n";
                break;
            case PI_BAD_SPI_SPEED:
                log() << "Bad speed setting for SPI channel.\n";
                break;
            case PI_BAD_FLAGS:
                log() << "Bad flags for configuring SPI channel.\n";
                break;
            case PI_NO_AUX_SPI:
                log() << "Auxilary SPI channel not available.\n";
                break;
            case PI_SPI_OPEN_FAILED:
                log() << "Failed to open SPI channel.\n";
                break;
            default:
                log() << "Unknown error (" << fd_ << ") on opening SPI channel.\n";
                break;
            }
            throw std::runtime_error("Failed to open SPI channel via pigpiod.");
        }
    }
#endif
}

void Zero2WSPI::close()
{
#if defined(SPI_SPIDEV)
    if (fd_ >= 0) {
        if (verbose()) {
            log() << "Closing SPI interface: " << interface_ << std::endl;
        }
        ::close(fd_);
    }
#endif
#if defined(SPI_PIGPIO)
    if (fd_ >= 0) {
        if (verbose()) {
            log() << "Closing SPI channel.\n";
        }
        spi_close(channel_, fd_);
        fd_ = -1;
    }
    if (channel_ >= 0) {
        if (verbose()) {
            log() << "Closing channel to pigpiod.\n";
        }
        pigpio_stop(channel_);
        channel_ = -1;
    }
#endif
}


void Zero2WSPI::select()
{
    if (verbose()) {
        log() << "Zero2WSPI::select() not implemented\n";
    }
}

void Zero2WSPI::deselect()
{
    if (verbose()) {
        log() << "Zero2WSPI::deselect() not implemented\n";
    }
}

void Zero2WSPI::writeAll(std::array<uint8_t, 2> const &value)
{
#if defined(SPI_SPIDEV)
    writeBlocking_spidev([value](unsigned) { return value; });
#endif
#if defined(SPI_PIGPIO)
    writeBlocking_pigpiod([value](unsigned) { return value; });
#endif
}

void Zero2WSPI::writeAll(std::function<std::array<uint8_t, 2>(unsigned)> const &value)
{
#if defined(SPI_SPIDEV)
    writeBlocking_spidev(value);
#endif
#if defined(SPI_PIGPIO)
    writeBlocking_pigpiod(value);
#endif
}

void Zero2WSPI::writeBlocking_spidev(std::function<std::array<uint8_t, 2>(unsigned)> const &value)
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
        log() << "Writing " << tx_buf.size() << " (" << bufSize << ") bytes: ";
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

void Zero2WSPI::writeBlocking_pigpiod(std::function<std::array<uint8_t, 2>(unsigned)> const &value)
{
    if (fd_ < 0) {
        open();
    }
    const size_t bufSize{numModules() * 2};
    std::vector<uint8_t> tx_buf;
    tx_buf.resize(bufSize, 0);
    for (unsigned i = 0; i < numModules(); ++i)
    {
        auto const &bytes = value(i);
        tx_buf[i * 2] = bytes[0];
        tx_buf[1 + i * 2] = bytes[1];
    }

    if (verbose()) {
        log() << "Writing " << tx_buf.size() << " (" << bufSize << ") bytes: ";
        for (auto const &byte : tx_buf)
            log()  << hex(byte >> 4) << hex(byte & 0x0f) << " ";
        log() << std::endl;
    }
    auto result = spi_write(channel_, fd_, reinterpret_cast<char*>(tx_buf.data()), tx_buf.size());
    if (result < 0) {
        switch (result) {
        case PI_BAD_HANDLE:
            log() << "Bad SPI channel #.\n";
            break;
        case PI_BAD_SPI_COUNT:
            log() << "Bad SPI channel #.\n";
            break;
        case PI_SPI_XFER_FAILED:
            log() << "Failed to send all data to SPI channel.\n";
            break;
        default:
            log() << "Unknown error (" << result << ") on writing data to SPI channel.\n";
            break;
        }
    }
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
