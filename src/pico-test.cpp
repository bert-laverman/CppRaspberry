// Copyright 2023 by Bert Laverman, All rights reserved.

#if defined(TARGET_PICO)

#include "PICO.hpp"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"

#include "pico/pico.hpp"
using nl::rakis::raspberry::PICO;

#else

#include "zero2w/zero2w.hpp"
using nl::rakis::raspberry::Zero2W;

#endif

#include "devices/max7219.hpp"

#include <stdio.h>

using nl::rakis::raspberry::devices::MAX7219;

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

// int64_t alarm_callback([[maybe_unused]] alarm_id_t id, [[maybe_unused]] void *user_data) {
//     // Put your timeout handler code in here
//     return 0;
// }

template <typename T>
void setup(T& berry) {
    berry.spi().deselect();
    
    // I2C Initialisation. Using it at 400Khz.
    //i2c_init(I2C_PORT, 400*1000);
        /*
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
        */


    // Timer example code - This example fires off the callback after 2000ms
    //add_alarm_in_ms(2000, alarm_callback, NULL, false);
}

int main()
{
    //PICO berry;
    Zero2W berry;
    setup(berry);
    berry.spi().deselect();
    berry.spi().numModules(2);
    berry.spi().open();

    MAX7219 max7219(berry.spi());
    max7219.shutdown();
    max7219.displayTest(0);
    max7219.setScanLimit(7);
    max7219.setDecodeMode(255);
    max7219.startup();
    max7219.setBrightness(8);

    max7219.setNumber(0, 10);
    //max7219.setNumber(0, 12345678);
    //max7219.setNumber(1, 87654321);

    return 0;
}
