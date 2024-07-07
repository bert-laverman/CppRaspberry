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


#include <cstring>
#include <format>
#include <exception>

extern "C" {
#include <pigpiod_if2.h>
}

#include <interfaces/pigpiod-i2c.hpp>


using namespace nl::rakis::raspberrypi::interfaces;


PigpiodI2C::~PigpiodI2C()
{
    close();
}

void PigpiodI2C::error(int result, int bus, uint8_t address, unsigned size) {
    switch (result) {
    case PI_BAD_I2C_ADDR:
        log(std::format("Bad I2C address 0x{:02x}", address));
        break;
    case PI_BAD_I2C_BUS:
        log(std::format("Bad I2C bus number {}", bus));
        break;
    case PI_BAD_FLAGS:
        log("Bad I2C flags");
        break;
    case PI_BAD_HANDLE:
        log("Bad I2C handle");
        break;
    case PI_BAD_PARAM:
        log("Bad I2C parameter");
        break;
    case PI_I2C_OPEN_FAILED:
        log("I2C open failed");
        break;
    case PI_I2C_WRITE_FAILED:
        log(std::format("Failed to write {} bytes to address 0x{:02x}", size, address));
        break;
    case PI_NO_HANDLE:
        log("No I2C handle");
        break;
    default:
        log(std::format("Failed to open I2C handle for address 0x{:02x}. (error {})", address, result));
        break;
    }
}

void PigpiodI2C::open()
{
    if (initialized()) {
        return;
    }
    if (channel() < 0) {
        log("Opening channel to pigpiod.");
        channel(pigpio_start(nullptr, nullptr));
        if (channel() < 0) {
            log(std::format("Failed to open I2C channel: {}", channel()));

            return;
        }
    }
    initialized(true);
}

bool PigpiodI2C::canListen() const noexcept {
    return false;
}


static constexpr const char* controllerOnly = "PigpiodI2C only supports Controller mode.";

void PigpiodI2C::startListening()
{
    log(controllerOnly);
    throw new std::runtime_error(controllerOnly);
}

void PigpiodI2C::stopListening()
{
    log(controllerOnly);
}

void PigpiodI2C::close() {
    if (!initialized()) {
        return;
    }

    if (channel() >= 0) {
        log(std::format("Closing channel {}", channel()));
        pigpio_stop(channel());
        channel(-1);
    }
    initialized(false);
}

bool PigpiodI2C::canSend() const noexcept {
    return true;
}

bool PigpiodI2C::write(uint8_t address, std::span<uint8_t> data)
{
    open();

    if (data.empty()) {
        log("writeBytes(): No data to write");

        return true;
    }
    int handle{ -1 };
    bool success{ false };
    try {
        log(std::format("Opening bus {} on channel {} for address 0x{:02x}", bus(), channel(), address));
        handle = i2c_open(channel(), bus(), address, 0);
        if (handle >= 0) {
            auto result = i2c_write_device(channel(), handle, reinterpret_cast<char*>(data.data()), data.size());
            if (result >= 0) {
                success = true;
            } else {
                error(result, bus(), address, data.size());
            }
        } else if (verbose()) {
            error(handle);
        }
    } catch (...) {
        log("Exception caught in writeBytes()");
    }
    if (handle >= 0) {
        i2c_close(channel(), handle);
    }

    return success;
}
