// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47

#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "pico/error.h"
#include "pico/types.h"

#include <vector>
#include <iostream>

#include <interfaces/pico_i2c.hpp>

#if !defined(TARGET_PICO) && !defined(HAVE_I2C)
#error "This file is for the Pico with I2C enabled only!"
#endif


using nl::rakis::raspberrypi::protocols::MsgHeader;
using namespace nl::rakis::raspberrypi::interfaces;


PicoI2C::PicoI2C(i2c_inst_t *interface, unsigned sdaPin, unsigned sclPin)
    : I2C(sdaPin, sclPin), interface_(interface)
{
}

PicoI2C &PicoI2C::defaultInstance()
{
    static PicoI2C instance{i2c0, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN};
    return instance;
}


void PicoI2C::open()
{
    if (!initialized()) {
        printf("Initialising I2C channel %d on pins %d and %d\n", channel(), sdaPin(), sclPin());
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
            printf("Disabling I2C0 interrupts on channel %d.\n", channel());
            irq_set_enabled(I2C0_IRQ, false);
        } else if (interface_ == i2c1) {
            printf("Disabling I2C1 interrupts on channel %d.\n", channel());
            irq_set_enabled(I2C1_IRQ, false);
        }
        printf("Deinitialising I2C%d\n", channel());
        i2c_deinit(interface_);
        initialized(false);
    }
}


void PicoI2C::switchToControllerMode()
{
    printf("Switching to Controller mode on channel %d.\n", channel());
    reset();
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
    printf("I2C interrupt on channel %d. (status=0x%08lx)\n", picoI2C.channel(), status);
    if (status & I2C_IC_INTR_STAT_R_GEN_CALL_BITS) {
        printf("Clearing General Call on channel %d. (0x0%08lx)\n", picoI2C.channel(), i2c->hw->clr_gen_call);

        if (status & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
            // printf("I2C General call RX full on channel %d.\n", picoI2C.channel());
            MsgHeader header;
            if (!i2cReadByte(i2c, header.command) ||
                !i2cReadByte(i2c, header.length) ||
                !i2cReadByte(i2c, header.sender) ||
                !i2cReadByte(i2c, header.checksum))
            {
                printf("Timeout trying to receive message header on channel %d.\n", picoI2C.channel());
                return;
            }
            printf("I2C General Call payload on channel %d is %d byte(s), checksum 0x%02x.\n", picoI2C.channel(), header.length, header.checksum);
            std::vector<uint8_t> data(header.length, 0);
            if (!i2c_read_raw_blocking(i2c0, data)) {
                printf("Timeout trying to receive message data on I2C channel %d.\n", picoI2C.channel());
                return;
            }

            if (!verify_payload(header, data)) {
                printf("Checksum failed for General Call message on I2C channel %d.\n", picoI2C.channel());
                return;
            }

            if (picoI2C.callback()) {
                // printf("Calling I2C callback for channel %d.\n", picoI2C.channel());
                picoI2C.callback()(header.command, header.sender, data);
            } else {
                printf("No callback set for channel %d.\n", picoI2C.channel());
            }
        } else {
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
        printf("I2C message payload on channel %d is %d byte(s), checksum 0x%02x.\n", picoI2C.channel(), header.length, header.checksum);
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
            picoI2C.callback()(header.command, header.sender, data);
        } else {
            printf("No callback set for channel %d.\n", picoI2C.channel());
        }
    } else {
        printf("I2C unknown interrupt on channel %d.\n", picoI2C.channel());
    }
}

static void i2c0_cb() {
    i2c_cb(i2c0, *picoI2C0);
}

static void i2c1_cb() {
    i2c_cb(i2c1, *picoI2C1);
}

void PicoI2C::switchToResponderMode(uint8_t address, MsgCallback cb)
{
    printf("Switching to responder mode on channel %d using address 0x%02x\n", channel(), address);

    listenAddress(address);
    callback(cb);
    reset();

    if (interface_ == i2c0) {
        printf("Setting up IRQ handler for i2c0\n");
        picoI2C0 = this;
        irq_set_exclusive_handler(I2C0_IRQ, i2c0_cb);
        i2c_set_slave_mode(interface_, true, address);
        i2c0->hw->intr_mask = I2C_IC_INTR_STAT_R_RX_FULL_BITS|I2C_IC_INTR_STAT_R_GEN_CALL_BITS;
        irq_set_enabled(I2C0_IRQ, true);
    } else if (interface_ == i2c1) {
        printf("Setting up IRQ handler for i2c1\n");
        picoI2C1 = this;
        irq_set_exclusive_handler(I2C1_IRQ, i2c1_cb);
        i2c_set_slave_mode(interface_, true, address);
        i2c1->hw->intr_mask = I2C_IC_INTR_STAT_R_RX_FULL_BITS|I2C_IC_INTR_STAT_R_GEN_CALL_BITS;
        irq_set_enabled(I2C1_IRQ, true);
    } else {
        log() << "Unknown I2C interface" << std::endl;
    }
}

bool PicoI2C::readBytes(uint8_t address, std::span<uint8_t> data)
{
    printf("PicoI2C::readBytes(address=0x%02x, data.size=%d)\n", address, data.size());
    return false;
}

bool PicoI2C::writeBytes(uint8_t address, std::span<uint8_t> data)
{
    printf("Sending %d bytes to 0x%02x on channel %d.\n", data.size(), address, channel());

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