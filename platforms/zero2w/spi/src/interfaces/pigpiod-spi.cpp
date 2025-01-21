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

#include <raspberry-pi.hpp>
#include <interfaces/pigpiod-spi.hpp>

using namespace nl::rakis::raspberrypi::interfaces;


void PigpiodSPI::open()
{
    auto& gpio = RaspberryPi::gpio();
    switch (busNr_) {
    case 0:
        switch (csNr_) {
        case 0:
            csPin(8);
            break;
        case 1:
            csPin(7);
            break;
        default:
            log(std::format("Non-standard CS line {} for bus {}.", csNr_, busNr_));
            break;
        }

        sclkPin(11);
        mosiPin(10);
        if (is4Pin()) {
            misoPin(9);
        }
        break;

    case 1:
        switch (csNr_) {
        case 0:
            csPin(18);
            break;
        case 1:
            csPin(17);
            break;
        case 2:
            csPin(16);
            break;
        default:
            log(std::format("Non-standard CS line {} for bus {}.", csNr_, busNr_));
            break;
        }

        sclkPin(21);
        mosiPin(20);
        if (is4Pin()) {
            misoPin(19);
        }
        break;

    default:
        log(std::format("Non-standard bus nr {}.", busNr_));
        break;
    }

    // Claim Chip-select
    if (csPin() == NO_PIN) {
        throw new std::runtime_error(std::format("No CS pin set for SPI bus {}, cs line {}", busNr_, csNr_));
    }
    gpio.claim(csPin());

    // Claim Clock
    if (sclkPin() == NO_PIN) {
        throw new std::runtime_error(std::format("No SCLK pin set for SPI bus {}", busNr_));
    }
    gpio.claim(sclkPin(), GPIOMode::SPI);

    // Claim MOSI
    if (mosiPin() == NO_PIN) {
        throw new std::runtime_error(std::format("No MOSI pin set for SPI bus {}", busNr_));
    }
    gpio.claim(mosiPin(), GPIOMode::SPI);
    if (is4Pin()) {
        // Claim MISO
        if (misoPin() == NO_PIN) {
            throw new std::runtime_error(std::format("No MISO pin set for SPI bus {}", busNr_));
        }
        gpio.claim(misoPin(), GPIOMode::SPI);
    }

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


void PigpiodSPI::write(std::span<uint8_t> data)
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
}
