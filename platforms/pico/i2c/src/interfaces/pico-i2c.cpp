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


#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "pico/error.h"
#include "pico/types.h"

#include <vector>
#include <iostream>

#include <protocols/messages.hpp>
#include <interfaces/pico-i2c.hpp>


#if !defined(TARGET_PICO) && !defined(HAVE_I2C)
#error "This file is for the Pico with I2C enabled only!"
#endif


using namespace nl::rakis::raspberrypi::interfaces;
using namespace nl::rakis::raspberrypi::protocols;


PicoI2C::PicoI2C(i2c_inst_t *interface, unsigned sdaPinn, unsigned sclPinn)
    : interface_(interface)
{
    sdaPin(sdaPinn);
    sclPin(sclPinn);
}


void PicoI2C::open()
{
    if (!initialized()) {
        if (verbose()) {
            printf("Initialising I2C channel %d on pins %d and %d\n", channel(), sdaPin(), sclPin());
        }
        i2c_init(interface_, baudrate);

        gpio_set_function(sdaPin(), GPIO_FUNC_I2C);
        gpio_set_function(sclPin(), GPIO_FUNC_I2C);
        gpio_pull_up(sdaPin());
        gpio_pull_up(sclPin());

        initialized(true);
    }
}

void PicoI2C::close()
{
    if (initialized()) {
        if (interface_ == i2c0) {
            if (verbose()) {
                printf("Disabling I2C0 interrupts on channel %d.\n", channel());
            }
            irq_set_enabled(I2C0_IRQ, false);
        } else if (interface_ == i2c1) {
            if (verbose()) {
                printf("Disabling I2C1 interrupts on channel %d.\n", channel());
            }
            irq_set_enabled(I2C1_IRQ, false);
        }
        if (verbose()) {
            printf("Deinitialising I2C%d\n", channel());
        }
        i2c_deinit(interface_);
        initialized(false);
    }
}


bool PicoI2C::canListen() const noexcept
{
    return true;
}


/**
 * @brief Read a byte from the I2C channel, waiting at most a certain number of microseconds
 */
static bool i2cReadByte(i2c_inst_t* i2c, uint8_t& byte, uint32_t timeout_us = 500)
{
    auto limit = time_us_64() + timeout_us;
    while (i2c_get_read_available(i2c) == 0) {
        if (time_us_64() > limit) {
            return false;
        }
    }
    byte = i2c_read_byte_raw(i2c);
    return true;
}

/**
 * @brief Read a number of bytes from the I2C channel, waiting at most a certain number of microseconds
 */
static bool i2c_read_raw_blocking(i2c_inst_t* i2c, std::vector<uint8_t>& data, uint32_t timeout_us = 500)
{
    for (auto& byte : data) {
        if (!i2cReadByte(i2c, byte, timeout_us)) {
            return false;
        }
    }
    return true;
}


/**
 * @brief Verify the payload given the provided header.
 */
static bool verify_payload(MsgHeader const& header, std::vector<uint8_t> const& data)
{
    if (header.length != data.size()) {
        printf("Verify payload: Invalid length\n");
        return false;
    }

    uint8_t checksum = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        checksum ^= data[i];
    }

    if (checksum != header.checksum) {
        printf("Verify payload: Invalid checksum\n");
        return false;
    }

    return true;
}

/**
 * @brief I2C0 instance
 */
static PicoI2C* picoI2C0 = nullptr;
static PicoI2C* picoI2C1 = nullptr;


/**
 * @brief I2C0 interrupt handler
 */
static void i2c_cb(i2c_inst_t* i2c, PicoI2C& picoI2C) {
    auto status = i2c->hw->intr_stat;
    if (status == 0) {
        return;
    }
    if (picoI2C.verbose()) {
        printf("I2C interrupt on channel %d. (status=0x%08lx)\n", picoI2C.channel(), status);
    }
    if (status & I2C_IC_INTR_STAT_R_GEN_CALL_BITS) {
        auto gcStatus = i2c->hw->clr_gen_call;
        // if (picoI2C.verbose()) {
            printf("Clearing General Call on channel %d. (0x0%08lx)\n", picoI2C.channel(), gcStatus);
        // }
        if (status & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
            MsgHeader header;
            if (!i2cReadByte(i2c, header.command) ||
                !i2cReadByte(i2c, header.length) ||
                !i2cReadByte(i2c, header.sender) ||
                !i2cReadByte(i2c, header.checksum))
            {
                printf("Timeout trying to receive GC message header on channel %d.\n", picoI2C.channel());
                return;
            }
            if (picoI2C.verbose()) {
                printf("I2C General Call payload on channel %d is %d byte(s), checksum 0x%02x.\n", picoI2C.channel(), header.length, header.checksum);
            }
            std::vector<uint8_t> data(header.length, 0);
            if (!i2c_read_raw_blocking(i2c0, data)) {
                printf("Timeout trying to receive GC message data on I2C channel %d.\n", picoI2C.channel());
                return;
            }

            if (!verify_payload(header, data)) {
                printf("Checksum failed for GC message on I2C channel %d.\n", picoI2C.channel());
                return;
            }

            if (picoI2C.callback()) {
                // printf("Calling I2C callback for channel %d.\n", picoI2C.channel());
                picoI2C.callback()(toCommand(header.command), header.sender, data);
            } else {
                printf("No callback set for channel %d.\n", picoI2C.channel());
            }
        } else if (picoI2C.verbose()) {
            printf("I2C General Call announced on channel %d.\n", picoI2C.channel());
        }
    } else if (status & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
        // printf("I2C RX full on channel %d.\n", picoI2C.channel());

        MsgHeader header;
        if (!i2cReadByte(i2c, header.command) ||
            !i2cReadByte(i2c, header.length) ||
            !i2cReadByte(i2c, header.sender) ||
            !i2cReadByte(i2c, header.checksum))
        {
                printf("Timeout trying to receive message header on channel %d.\n", picoI2C.channel());
            return;
        }
        if (picoI2C.verbose()) {
            printf("I2C message payload on channel %d is %d byte(s), checksum 0x%02x.\n", picoI2C.channel(), header.length, header.checksum);
        }
        std::vector<uint8_t> data(header.length, 0);
        if (!i2c_read_raw_blocking(i2c, data)) {
            printf("Timeout trying to receive message data on I2C channel %d.\n", picoI2C.channel());
            return;
        }

        if (!verify_payload(header, data)) {
            printf("Checksum failed for message on I2C channel %d.\n", picoI2C.channel());
            return;
        }

        if (picoI2C.callback()) {
            // printf("Calling I2C callback for channel %d.\n", picoI2C.channel());
            picoI2C.callback()(toCommand(header.command), header.sender, data);
        } else {
            printf("No callback set for channel %d.\n", picoI2C.channel());
        }
    } else if (picoI2C.verbose()) {
        printf("I2C unknown interrupt on channel %d.\n", picoI2C.channel());
    }
}

static void i2c0_cb() {
    i2c_cb(i2c0, *picoI2C0);
}

static void i2c1_cb() {
    i2c_cb(i2c1, *picoI2C1);
}

void PicoI2C::startListening()
{
    if (listening()) {
        return;
    }
    if (verbose()) {
        printf("Switching to responder mode on channel %d using address 0x%02x\n", channel(), listenAddress());
    }
    reset();

    if (interface_ == i2c0) {
        if (verbose()) {
            printf("Setting up IRQ handler for i2c0\n");
        }
        picoI2C0 = this;
        irq_set_exclusive_handler(I2C0_IRQ, i2c0_cb);
        i2c_set_slave_mode(interface_, true, listenAddress());
        i2c0->hw->intr_mask = I2C_IC_INTR_STAT_R_RX_FULL_BITS|I2C_IC_INTR_STAT_R_GEN_CALL_BITS;
        irq_set_enabled(I2C0_IRQ, true);
    } else if (interface_ == i2c1) {
        if (verbose()) {
            printf("Setting up IRQ handler for i2c1\n");
        }
        picoI2C1 = this;
        irq_set_exclusive_handler(I2C1_IRQ, i2c1_cb);
        i2c_set_slave_mode(interface_, true, listenAddress());
        i2c1->hw->intr_mask = I2C_IC_INTR_STAT_R_RX_FULL_BITS|I2C_IC_INTR_STAT_R_GEN_CALL_BITS;
        irq_set_enabled(I2C1_IRQ, true);
    } else {
        log() << "Trying to use an unknown I2C interface" << std::endl;
    }
}


void PicoI2C::stopListening()
{
    if (!listening()) {
        return;
    }
    if (verbose()) {
        printf("Switching to master mode on channel %d\n", channel());
    }
    if (interface_ == i2c0) {
        irq_set_enabled(I2C0_IRQ, false);
        picoI2C0 = nullptr;
    } else if (interface_ == i2c1) {
        irq_set_enabled(I2C1_IRQ, false);
        picoI2C1 = nullptr;
    } else {
        log() << "Trying to use an unknown I2C interface" << std::endl;
    }
    i2c_set_slave_mode(interface_, false, 0);
}

bool PicoI2C::canSend() const noexcept
{
    return true;
}

bool PicoI2C::write(uint8_t address, std::span<uint8_t> data)
{
    if (verbose()) {
        printf("Sending %d bytes to 0x%02x on channel %d.\n", data.size(), address, channel());
    }
    [[maybe_unused]]
    absolute_time_t deadline{ time_us_64() + (5000 * data.size()) };
    auto result = i2c_write_blocking_until(interface_, address, data.data(), data.size(), false, deadline);
    if (result == PICO_ERROR_GENERIC) {
        printf("Failed to write bytes to 0x%02x. No one there.\n", address);
        return false;
    } else if (result == PICO_ERROR_TIMEOUT) {
        printf("Failed to write bytes to 0x%02x. Timeout.\n", address);
        return false;
    }  else if (result < 0) {
        printf("Failed to write bytes to 0x%02x. Errno=%d\n", address, result);
        return false;
    } else if (static_cast<unsigned>(result) != data.size()) {
        printf("Failed to write %d bytes to 0x%02x. Only wrote %d bytes.\n", data.size(), address, result);
        return false;
    }
    return true;
}