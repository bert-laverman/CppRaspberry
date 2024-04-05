// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47

#if !defined(TARGET_PICO) && !defined(HAVE_I2C)
#error "This file is for the Pico with I2C enabled only!"
#endif

#include <vector>
#include <iostream>

#include <pico/interfaces/pico_i2c.hpp>

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
        printf("Initialising I2C on pins %d and %d\n", sdaPin(), sclPin());
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
            printf("Disabling I2C0 interrupts\n");
            irq_set_enabled(I2C0_IRQ, false);
        } else if (interface_ == i2c1) {
            printf("Disabling I2C1 interrupts\n");
            irq_set_enabled(I2C1_IRQ, false);
        }
        printf("Deinitialising I2C%d\n", i2c_hw_index(interface_));
        i2c_deinit(interface_);
        initialized(false);
    }
}


void PicoI2C::switchToControllerMode()
{
    printf("Switching to Controller mode\n");
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
        printf("Invalid length\n");
        return false;
    }

    uint8_t checksum = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        checksum ^= data[i];
    }

    if (checksum != header.checksum) {
        printf("Invalid checksum\n");
        return false;
    }

    return true;
}

/**
 * @brief I2C0 instance
 */
static PicoI2C* picoI2C0 = nullptr;


/**
 * @brief I2C0 interrupt handler
 */
static void i2c0_cb() {
    auto status = i2c0->hw->intr_stat;
    printf("I2C0 interrupt (status=0x%04lx)\n", status);

    if (status & I2C_IC_INTR_STAT_R_GEN_CALL_BITS) {
        printf("Clearing General Call (0x0%8lx)\n", i2c0->hw->clr_gen_call);

        if (status & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
            printf("I2C0 General call RX full\n");
            MsgHeader header;
            if (!i2cReadByte(i2c0, header.command) ||
                !i2cReadByte(i2c0, header.length) ||
                !i2cReadByte(i2c0, header.sender) ||
                !i2cReadByte(i2c0, header.checksum))
            {
                printf("Timeout trying to receive message header.\n");
                return;
            }
            printf("I2C0 General Call payload is %d byte(s)\n", header.length);
            std::vector<uint8_t> data(header.length, 0);
            if (!i2c_read_raw_blocking(i2c0, data)) {
                printf("Timeout trying to receive message data.\n");
                return;
            }

            if (!verify_payload(header, data)) {
                return;
            }

            printf("Checking callback\n");
            if ((picoI2C0 != nullptr) && picoI2C0->callback()) {
                printf("Calling callback\n");
                picoI2C0->callback()(header.command, header.sender, data);
            }
        } else {
            printf("I2C0 General Call, no data yet\n");
        }
    } else if (status & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
        printf("I2C0 RX full\n");

        MsgHeader header;
        if (!i2cReadByte(i2c0, header.command) ||
            !i2cReadByte(i2c0, header.length) ||
            !i2cReadByte(i2c0, header.sender) ||
            !i2cReadByte(i2c0, header.checksum))
        {
            printf("Timeout trying to receive message header.\n");
            return;
        }
        printf("I2C0 payload is %d byte(s)\n", header.length);
        std::vector<uint8_t> data(header.length, 0);
        if (!i2c_read_raw_blocking(i2c0, data)) {
            printf("Timeout trying to receive message data.\n");
            return;
        }

        if (!verify_payload(header, data)) {
            return;
        }

        printf("Checking callback\n");
        if ((picoI2C0 != nullptr) && picoI2C0->callback()) {
            printf("Calling callback\n");
            picoI2C0->callback()(header.command, header.sender, data);
        }
    } else if (status == 0) {
        printf("I2C0 zero interrupt\n");
    } else {
        printf("I2C0 unknown interrupt\n");
    }
}

void PicoI2C::switchToResponderMode(uint8_t address, MsgCallback cb)
{
    printf("Switching to responder mode on address 0x%02x\n", address);

    listenAddress(address);
    callback(cb);
    reset();

    if (interface_ == i2c0) {
        printf("Setting up IRQ handler for i2c0\n");
        picoI2C0 = this;
        irq_set_exclusive_handler(I2C0_IRQ, i2c0_cb);
    }
    else {
        log() << "Unknown I2C interface" << std::endl;
    }
    i2c_set_slave_mode(interface_, true, address);
    i2c0->hw->intr_mask = I2C_IC_INTR_STAT_R_RX_FULL_BITS|I2C_IC_INTR_STAT_R_GEN_CALL_BITS;
    irq_set_enabled(I2C0_IRQ, true);
}
