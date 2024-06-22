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
#include <format>

extern "C" {
#include <pigpiod_if2.h>
}

#include <interfaces/pigpiod-spi.hpp>

using namespace nl::rakis::raspberrypi::interfaces;



void PigpiodSPI::validate()
{

}

bool PigpiodSPI::selected() const noexcept {
    return false;
}

void PigpiodSPI::open()
{
    if (channel_ < 0) {
        log("Opening a channel to pigpiod.");
        channel_ =  pigpio_start(nullptr, nullptr);
        if (channel_ < 0) {
            log() << "Failed to open a channel to pigpiod.\n";
            throw std::runtime_error("Failed to open a channel to pigpiod.");
        }
    }
    if (fd_ < 0) {
        log(std::format("Opening SPI bus {} at {} BAUD", busNr_, baudRate()));
        fd_ =  spi_open(channel_, busNr_, baudRate(), 0x0000);
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
}

void PigpiodSPI::close()
{
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
}


void PigpiodSPI::select()
{
    if (verbose()) {
        log() << "PigpiodSPI::select() not implemented\n";
    }
}

void PigpiodSPI::deselect()
{
    if (verbose()) {
        log() << "PigpiodSPI::deselect() not implemented\n";
    }
}

void PigpiodSPI::write(std::span<uint8_t> data)
{
    writeBlocking(data);
}

void PigpiodSPI::writeBlocking(std::span<uint8_t> data)
{
    if (fd_ < 0) {
        open();
    }

    if (verbose()) {
        log() << std::format("Writing {} bytes: ", data.size());
        for (auto const &byte : data)
            log() << std::format("0x{:02x} ", byte);
        log() << std::endl;
    }
    auto result = spi_write(channel_, fd_, reinterpret_cast<char*>(data.data()), data.size());
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
