// Copyright 2023 by Bert Laverman, All rights reserved.

#include <chrono>
#include <string>
#include <vector>
#include <iostream>

#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

#include <raspberry_pi.hpp>
#include <devices/max7219.hpp>

using nl::rakis::raspberrypi::RaspberryPi;
using nl::rakis::raspberrypi::interfaces::SPI;
using nl::rakis::raspberrypi::devices::MAX7219;

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

[[maybe_unused]]
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

static std::array<std::function<void(uint, uint32_t)>, 32> gpioCallbacks;

void gpioIRQ(uint gpio, uint32_t events) {
    if ((gpio < 32) && gpioCallbacks[gpio]) {
        gpioCallbacks[gpio](gpio, events);
    }
}

static std::array<bool, 32> gpioState;
static std::array<std::chrono::time_point<std::chrono::system_clock>, 32> lastDown;
static uint lastPin;
static uint clockFirst;
static uint counterClockFirst;

//static int value = 0;

uint gpioInitForInput(uint pin, std::function<void(uint, uint32_t)> callback) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpioCallbacks[pin] = callback;
    gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, gpioIRQ);
    gpioState[pin] = gpio_get(pin) != 0;

    return pin;
}

void printGpioStatus(uint pin, [[maybe_unused]] uint32_t events) {
//    auto now = std::chrono::system_clock::now();
    auto state = gpio_get(pin) != 0;

    if (state == gpioState[pin]) {
        return;
    }
    gpioState[pin] = state;

    if (pin != lastPin) {
        lastPin = pin;

        if ((pin == clockFirst) && state) {
            if (gpioState[counterClockFirst]) {
                printf("Up\n");
            }
        } else if ((pin == counterClockFirst) && state) {
            if (gpioState[clockFirst]) {
                printf("Down\n");
            }
        }
    }
}

static uint ledPin = PICO_DEFAULT_LED_PIN;
static uint rgbRPin = 0;
static uint rgbGPin = 1;
static uint rgbBPin = 2;

void setLed([[maybe_unused]] uint pin, uint32_t events) {
    if ((events & GPIO_IRQ_EDGE_RISE) != 0) {
        gpio_put(ledPin, 1);
    }
    if ((events & GPIO_IRQ_EDGE_FALL) != 0) {
        gpio_put(ledPin, 0);
    }
}

static void pwm_init_pin(uint8_t pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.f);
    pwm_init(slice_num, &config, true);
}

struct RGB {
    uint16_t r;
    uint16_t g;
    uint16_t b;
};

static void setColor(const RGB& color) {
    printf("RGB = %d, %d, %d\n", color.r, color.g, color.b );

    pwm_set_gpio_level(rgbRPin, color.r << 8);
    pwm_set_gpio_level(rgbGPin, color.g << 8);
    pwm_set_gpio_level(rgbBPin, color.b << 8);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] const char **argv)
{
 #if defined(TARGET_PICO)
    stdio_init_all();

    sleep_ms(1000);
    printf("Starting Pico test\n");

    printf("Set up internal LED\n");
    gpio_init(ledPin);
    gpio_set_dir(ledPin, GPIO_OUT);

    printf("Set up encoder pins\n");
    [[maybe_unused]] const auto button = gpioInitForInput(22, printGpioStatus);
    clockFirst = gpioInitForInput(26, printGpioStatus);
    counterClockFirst = gpioInitForInput(27, printGpioStatus);

    printf("Set up RGB LED\n");
    pwm_init_pin(rgbRPin);
    pwm_init_pin(rgbGPin);
    pwm_init_pin(rgbBPin);

    std::array<RGB, 8> colors{
        RGB{ 0, 0, 0 },
        RGB{ 255, 0, 0 },
        RGB{ 0, 255, 0 },
        RGB{ 0, 0, 255 },
        RGB{ 255, 255, 0 },
        RGB{ 0, 255, 255 },
        RGB{ 255, 0, 255 },
        RGB{ 255, 255, 255 }
    };

    bool blinker{ false };
    uint index{ 0 };

    printf("Start loop\n");
    while (true) {
        setColor(colors[index]);
        index = (index + 1) % colors.size();

        blinker = !blinker;
        gpio_put(ledPin, blinker);
        sleep_ms(1000);
    }

#else

   Options options;
    if (!parseArgs(options, argc, argv)) {
        return 1;
    }

    RaspberryPi& berry(*RaspberryPi::instance(true));
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
            berry.sleepMs(1000);
            --count;
        }
    } else {
        std::cerr << "Unknown command '" << options.command << "'\n";
        return 1;
    }
    max7219.sendData();
#endif
    return 0;
}
