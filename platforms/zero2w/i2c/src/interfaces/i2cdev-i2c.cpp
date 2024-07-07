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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

extern "C" {

#include <i2c/smbus.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

}

#include <interfaces/i2cdev-i2c.hpp>

/*
 * The I2C interface on the non-Pico Raspberry Pi's is a mess. Most libraries provide the Controller side of the protocol,
 * but not the Listener side. The pigpio library does provide the Listener side, but that is not compatible with its Controller
 * implementation. It even uses different pins for the SDA and SCL lines: The Controller side uses GPIO pins 2 and 3 for the
 * first (not zeroeth) I2C bus, while the Listener side uses GPIO pins 18 and 19. This is because it uses the hardware I2C
 * support on the Broadcom SoC, which is somehow also tied to the SPI support, and uses the PCM_FS and PCM_CLK pins.
 *
 * Note that this implementation uses pigpiod_if2.h, which is the C interface to the pigpiod daemon. This daemon must be
 * installed and running so we won't have to run as root. The pigpio library is a C library, so we have to use the C interface.
 *
 * For the Controller side I chose i2c-dev, which is the standard Linux I2C interface. This is a bit of a kludge, but it works.
 */

using nl::rakis::raspberrypi::protocols::MsgHeader;
using nl::rakis::raspberrypi::protocols::MsgHeaderSize;
using namespace nl::rakis::raspberrypi::interfaces;

/*
 * The I2CDevI2C class is an implementation of the Zero2WI2C interface using the i2c-dev library.
 * It only provide the I2C Controller functionality, using the ioctl() calls.
 */


I2CDevI2C::~I2CDevI2C()
{
    close();
}

/**
 * Opens the I2C interface specified by `interface_`.
 *
 * If the interface is already open, this function simply returns.
 * Otherwise, it attempts to open the interface using `::open()`. If the
 * open operation is successful, the `initialized()` flag is set to `true`.
 * If the open operation fails, the `fd_` member is set to `-1` and the
 * `initialized()` flag is set to `false`.
 *
 * This function is used internally by the `I2CDevI2C` class to
 * manage the lifetime of the I2C interface.
 */
void I2CDevI2C::open() {
    if (initialized()) {
        return;
    }


    log(std::format("Opening '{}'.", interface_));
    fd_ = ::open(interface_.c_str(), O_RDWR);
    if (fd_ < 0) {
        log(std::format("Failed to open '{}'. Errno={}.", interface_, errno));
    } else {
        initialized(true);
    }
}

void I2CDevI2C::close() {
    if (!initialized()) {
        return;
    }
    if (fd_ >= 0) {
        log(std::format("Closing '{}'.", interface_));
        ::close(fd_);
        fd_ = -1;
    }
    initialized(false);
}

bool I2CDevI2C::canListen() const noexcept
{
    return false;
}

static constexpr const char* controllerOnly = "I2CDevI2C only supports Controller mode.";

void I2CDevI2C::startListening()
{
    log(controllerOnly);
    throw new std::runtime_error(controllerOnly);
}

void I2CDevI2C::stopListening()
{
    log(controllerOnly);
}

bool I2CDevI2C::canSend() const noexcept
{
    return true;
}

bool I2CDevI2C::write(uint8_t address, std::span<uint8_t> data)
{
    open();

    log(std::format("Going to send {} bytes to 0x{:02x}.", data.size(), address));

    struct i2c_msg msg{ address, 0, static_cast<__u16>(data.size()), data.data() };
    struct i2c_rdwr_ioctl_data msgs{ &msg, 1 };

    auto result = ::ioctl(fd_, I2C_RDWR, &msgs);
    if (result == 0) {
        return true;
    }
    if (result != 1) {
        log(std::format("Failed to write {} bytes to 0x{:02x}. Errno={}.", data.size(), address, errno));

        return false;
    }
    return true;
}
