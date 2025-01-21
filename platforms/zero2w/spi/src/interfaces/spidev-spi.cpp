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

void SPIDevSPI::open()
{
    if (fd_ < 0) {
        log(std::format("Opening SPI device '{}'.", interface_));
        fd_ = ::open(interface_.c_str(), O_RDWR);
        if (fd_ < 0) {
            log() << "Failed to open SPI interface: " << strerror(errno) << std::endl;
            throw std::runtime_error("Failed to open SPI interface");
        }

        uint32_t mode{ 0 };
        log("Setting device to mode 0");
        check(ioctl(fd_, SPI_IOC_WR_MODE32, &mode));
        log("... checking");
        check(ioctl(fd_, SPI_IOC_RD_MODE32, &mode));
        if (mode != 0) {
            log("SPIdev does not support mode 0.");
        }

        uint8_t bits{ 8 };
        log("Setting bit-per-word to 8.");
        check(ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits));
        log("... checking");
        check(ioctl(fd_, SPI_IOC_RD_BITS_PER_WORD, &bits));
        if (bits != 8) {
            log("SPIdev does not support 8-bits.");
        }

        const unsigned int speed{ baudRate() };
        log(std::format("Setting speed to {} Hz ({} kHz)", baudRate(), baudRate() / 1000));
        check(ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed));
        log("... checking");
        check(ioctl(fd_, SPI_IOC_RD_MAX_SPEED_HZ, &speed));
        if (speed != baudRate()) {
            log(std::format("SPIdev does not support requested speed {} Hz ({} kHz)", baudRate(), baudRate() / 1000));
        }

        // log("Setting word byte-order.");
        // ioctl(fd_, SPI_IOC_WR_LSB_FIRST, 0);
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


void SPIDevSPI::write(std::span<uint8_t> data)
{
    if (fd_ < 0) {
        open();
    }
    const size_t bufSize{/*1 + */data.size()};
    std::vector<uint8_t> rx_buf;

    rx_buf.resize(bufSize);
    rx_buf.assign(bufSize, 0);


    if (verbose()) {
        log() << std::format("Writing {} bytes: ", data.size());
        for (auto const &byte : data)
            log() << std::format("0x{:02x} ", byte);
        log() << std::endl;
    }
    // ::write(fd_, data.data(), data.size());
    struct spi_ioc_transfer tr{
        .tx_buf = (unsigned long)(data.data()),
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
    check(ioctl(fd_, SPI_IOC_MESSAGE(1), &tr));
}
