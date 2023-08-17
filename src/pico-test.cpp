// Copyright 2023 by Bert Laverman, All rights reserved.

#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>

#include <raspberry_pi.hpp>
#include <devices/max7219.hpp>

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

struct Options {
    bool verbose;
    int numModules;
    int module;
    std::string command;
    std::vector<std::string> contents;
};

static bool parseArgs(Options& options, int argc, const char **argv) {
    options.verbose = false;
    options.numModules = 1;
    options.module = 0;
    options.command = "demo";
    options.contents.clear();

    int i = 1;
    for (; (i < argc) && (argv[i][0] == '-'); ++i) {
        std::string arg(argv[i]);
        if ((arg == "-v") || (arg == "--verbose")) {
            options.verbose = true;
        }
        else if ((arg == "-n") || (arg == "--num-modules")) {
            if (++i < argc) {
                options.numModules = atoi(argv[i]);
            }
            else {
                std::cerr << "Missing argument for -n\n";
                return false;
            }
        }
        else if ((arg == "-m") || (arg == "--module")) {
            if (++i < argc) {
                options.module = atoi(argv[i]);
            }
            else {
                std::cerr << "Missing argument for -m\n";
                return false;
            }
        }
        else {
            std::cerr << "Unknown option '" << arg << "'\n";
            return false;
        }
    }
    if (i < argc) { options.command = argv[i++]; }
    while (i < argc) { options.contents.push_back(argv[i++]); }
    return true;
}


int main(int argc, const char **argv)
{
    Options options;
    if (!parseArgs(options, argc, argv)) {
        return 1;
    }

    RaspberryPi& berry(*RaspberryPi::instance());
    berry.spi(0).numModules(options.numModules);

    MAX7219 max7219(berry.spi(0));
    max7219.shutdown();
    max7219.displayTest(0);
    max7219.setScanLimit(7);
    max7219.setDecodeMode(255);
    max7219.startup();
    max7219.setBrightness(8);
    max7219.writeImmediately(false);

    if (options.command == "clear") {
        if (options.module == 0) {
            for (int i = 0; i < options.numModules; ++i) {
                max7219.clear(i);
            }
        } else {
            max7219.clear(options.module-1);
        }
    } else if (options.command == "set") {
        if (options.module != 0) {
            max7219.clear(options.module-1);
            max7219.setNumber(options.module-1, atoi(options.contents[0].c_str()));
        } else {
            for (int i = 0; i < options.numModules; ++i) {
                max7219.clear(i);
                max7219.setNumber(i, atoi(options.contents[i].c_str()));
            }
        }
    } else if (options.command == "demo") {
        int count = 10000;
        while ( count > 0) {
            for (int i = 0; i < options.numModules; ++i) {
                max7219.setNumber(i, count);
            }
            max7219.sendData();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            --count;
        }
    } else {
        std::cerr << "Unknown command '" << options.command << "'\n";
        return 1;
    }
    max7219.sendData();

    return 0;
}
