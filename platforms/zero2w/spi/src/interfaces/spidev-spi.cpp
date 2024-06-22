/*
 * Copyright (c) 2024 by Bert Laverman. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdexcept>
#include <chrono>
#include <thread>

#include <interfaces/spidev-spi.hpp>


using namespace nl::rakis::raspberrypi::interfaces;


void SPIDevSPI::validate()
{

}

bool SPIDevSPI::selected() const noexcept {
    return false;
}

void SPIDevSPI::open()
{
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
}

void SPIDevSPI::close()
{
    if (fd_ >= 0) {
        if (verbose()) {
            log() << "Closing SPI interface: " << interface_ << std::endl;
        }
        ::close(fd_);
    }
}


void SPIDevSPI::select()
{
    if (verbose()) {
        log() << "SPIDevSPI::select() not implemented\n";
    }
}

void SPIDevSPI::deselect()
{
    if (verbose()) {
        log() << "SPIDevSPI::deselect() not implemented\n";
    }
}

void SPIDevSPI::write(std::span<uint8_t> value)
{
    writeBlocking(value);
}

void SPIDevSPI::writeBlocking(std::span<uint8_t> data)
{
    if (fd_ < 0) {
        open();
    }
    const size_t bufSize{/*1 + */data.size()};
    std::vector<uint8_t> tx_buf;
    std::vector<uint8_t> rx_buf;

    tx_buf.resize(bufSize);
    tx_buf.assign(bufSize, 0);
    rx_buf.resize(bufSize);
    rx_buf.assign(bufSize, 0);

    // tx_buf[0] = 0x40;
    for (unsigned i = 0; i < data.size(); ++i)
    {
        tx_buf[/*1 + */i] = data [i];
    }

    if (verbose()) {
        log() << std::format("Writing {} ({}) bytes: ", tx_buf.size(), bufSize);
        for (auto const &byte : tx_buf)
            log() << std::format("0x{:02x} ", byte);
        log() << std::endl;
    }
    struct spi_ioc_transfer tr{
        .tx_buf = (unsigned long)(tx_buf.data()),
        .rx_buf = (unsigned long)(rx_buf.data()),
        .len = bufSize,
        .speed_hz = baudRate(),
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
