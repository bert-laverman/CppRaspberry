// Copyright 2023 by Bert Laverman, All rights reserved.

#include <raspberry_pi.hpp>
#include <devices/max7219.hpp>

#include <stdio.h>

using nl::rakis::raspberry::RaspberryPi;
using nl::rakis::raspberry::interfaces::SPI;
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

int main(int argc, char **argv)
{
    RaspberryPi& berry(*RaspberryPi::instance());
    berry.spi(0).numModules((argc > 2) ? 2 : 1);

    MAX7219 max7219(berry.spi(0));
    max7219.shutdown();
    max7219.displayTest(0);
    max7219.setScanLimit(7);
    max7219.setDecodeMode(255);
    max7219.startup();
    max7219.setBrightness(8);

    for (unsigned i = 0; i < berry.spi(0).numModules(); i++) {
        max7219.clear(i);
    }
    if (argc > 2) {
        max7219.setNumber(0, atoi(argv[1]));
        max7219.setNumber(1, atoi(argv[2]));
    }
    else if (argc > 1)
    {
        max7219.setNumber(0, atoi(argv[1]));
    }
    else
    {
        max7219.setNumber(0, 12345678);
    }

    return 0;
}
