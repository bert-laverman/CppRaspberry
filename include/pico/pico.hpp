#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: General Pico management code

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "hardware/spi.h"

#include <raspberry_pi.hpp>
#include <pico/interfaces/pico_spi.hpp>

namespace nl::rakis::raspberrypi {

class PICO : public virtual RaspberryPi {
    interfaces::PicoSPI spi0_;

public:
    PICO(bool verbose =false) : RaspberryPi(verbose), spi0_() {
        if (verbose) {
            stdio_init_all();

            stdio_usb_init();
            while ( !stdio_usb_connected() ) tight_loop_contents();
            sleepMs(1000);
            log() << "Starting up...\n" << std::flush;
        }
    };

    virtual interfaces::SPI& spi([[maybe_unused]] unsigned num = 0) {
        return spi0_;
    };

    virtual interfaces::I2C& i2c([[maybe_unused]] unsigned num = 0) {
        throw std::runtime_error("I2C not implemented");
    };

    virtual std::ostream& log() {
        return std::cout;
    };
    
    virtual void sleepMs(unsigned ms) {
        sleep_ms(ms);
    };
};

} // namespace nl::rakis::raspberrypi