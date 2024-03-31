// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47

#if !defined(TARGET_PICO) && !defined(HAVE_I2C)
#error "This file is for the Pico with I2C enabled only!"
#endif

#include <vector>
#include <iostream>

#include <pico/interfaces/pico_i2c.hpp>

using namespace nl::rakis::raspberrypi::interfaces;

static PicoI2C* picoI2C0 = nullptr;
static void i2c0_cb() {
    auto status = i2c0->hw->intr_stat;
    printf("I2C0 interrupt (status=0x%04lx)\n", status);

    if (status & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
        printf("I2C0 RX full\n");
        auto len = i2c_read_byte_raw(i2c0);
        printf("I2C0 RX len %d\n", len);
        std::vector<uint8_t> data(len, 0);
        i2c_read_raw_blocking(i2c0, data.data(), len);
        printf("Checking callback\n");
        if ((picoI2C0 != nullptr) && picoI2C0->callback()) {
            printf("Calling callback\n");
            picoI2C0->callback()(picoI2C0->slaveAddress(), data);
        }
    }
}

void PicoI2C::switchToSlave(uint8_t address, SlaveCallback cb)
{
    printf("Switching to slave mode on address 0x%02x\n", address);

    slaveAddress(address);
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
    i2c0->hw->intr_mask = I2C_IC_INTR_STAT_R_RX_FULL_BITS;
    irq_set_enabled(I2C0_IRQ, true);
}
