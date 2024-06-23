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

#include <cstdint>

extern "C" {
#include <pigpiod_if2.h>
}

#include <atomic>
#include <format>
#include <string>
#include <exception>
#include <stdexcept>


#include <interfaces/gpio.hpp>


using namespace nl::rakis::raspberrypi::interfaces;


static constexpr unsigned int NumGPIO{ 28 };

static std::array<GPIOMode, NumGPIO> mode_{{
    GPIOMode::Unused,       // GPIO  0: Pin 27, default I2C-0 SDA (EEPROM Data)
    GPIOMode::Unused,       // GPIO  1: Pin 28, default I2C-0 SCL (EEPROM Clock)
    GPIOMode::Unused,       // GPIO  2: Pin  3, default I2C-1 SDA
    GPIOMode::Unused,       // GPIO  3: Pin  5, default I2C-1 SCL
    GPIOMode::Unused,       // GPIO  4: Pin  7, default GPCLK0
    GPIOMode::Unused,       // GPIO  5: Pin 29
    GPIOMode::Unused,       // GPIO  6: Pin 31
    GPIOMode::Unused,       // GPIO  7: Pin 26, default SPI-0 CE1
    GPIOMode::Unused,       // GPIO  8: Pin 24, default SPI-0 CE0
    GPIOMode::Unused,       // GPIO  9: Pin 21, default SPI-0 MISO
    GPIOMode::Unused,       // GPIO 10: Pin 19, default SPI-0 MOSI
    GPIOMode::Unused,       // GPIO 11: Pin 23, default SPI-0 SCLK
    GPIOMode::Unused,       // GPIO 12: Pin 32, default PWM0
    GPIOMode::Unused,       // GPIO 13: Pin 33, default PWM1
    GPIOMode::Unused,       // GPIO 14: Pin  8, default UART TX
    GPIOMode::Unused,       // GPIO 15: Pin 10, default UART RX
    GPIOMode::Unused,       // GPIO 16: Pin 36, default SPI-1 CE2
    GPIOMode::Unused,       // GPIO 17: Pin 11, default SPI-1 CE1
    GPIOMode::Unused,       // GPIO 18: Pin 12, default SPI-1 CE0
    GPIOMode::Unused,       // GPIO 19: Pin 35, default SPI-1 MISO
    GPIOMode::Unused,       // GPIO 20: Pin 38, default SPI-1 MOSI
    GPIOMode::Unused,       // GPIO 21: Pin 40, default SPI-1 SCLK
    GPIOMode::Unused,       // GPIO 22: Pin 15
    GPIOMode::Unused,       // GPIO 23: Pin 16
    GPIOMode::Unused,       // GPIO 24: Pin 18
    GPIOMode::Unused,       // GPIO 25: Pin 22
    GPIOMode::Unused,       // GPIO 26: Pin 37
    GPIOMode::Unused,       // GPIO 27: Pin 13
}};


inline static GPIOMode mode(unsigned pin) {
    return (pin >= NumGPIO) ? GPIOMode::Unavailable : mode_[pin];
}

bool GPIO::direct() const noexcept
{
    return true;
}

unsigned GPIO::numPins() const noexcept
{
    return NumGPIO;
}

bool GPIO::validPin(unsigned pin) const noexcept
{
    return mode(pin) != GPIOMode::Unavailable;
}

bool GPIO::used(unsigned pin) const noexcept
{
    return mode(pin) < GPIOMode::Unused;
}

bool GPIO::available(unsigned pin) const noexcept
{
    return mode(pin) == GPIOMode::Unused;
}

void GPIO::claim(unsigned pin, GPIOMode m)
{
    mode_[pin] = m;
}


static int gpioChannel{ -1 };

static void openChannel()
{
    if (gpioChannel < 0) {
        if ((gpioChannel = pigpio_start(nullptr, nullptr)) < 0) {
            std::cerr << "Failed to open the pigpiod channel.\n";
            throw std::runtime_error("Failed to open channel to pigpiod.\n");
        }
    }
}

static void closeChannel()
{
    if (gpioChannel >= 0) {
        pigpio_stop(gpioChannel);
        gpioChannel = -1;
    }
}

GPIO::GPIO()
{
    openChannel();
}

GPIO::~GPIO()
{
    closeChannel();
}

void GPIO::init(unsigned pin, GPIOMode m)
{
    if (m == GPIOMode::Unused) {
        if (used(pin)) {
            log(std::format("Releasing pin {}.", pin));
            mode_[pin] = m;
        } else {
            log(std::format("Releasing pin {}, but it already was unused.", pin));
        }
        return;
    }
    if (!available(pin)) {
        log(std::format("Pin {} is not available.", pin));
        throw new std::runtime_error("Pin not available.");
    }
    log(std::format("Claiming pin {}.", pin));

    mode_[pin] = m;
}

void GPIO::setForOutput(unsigned pin)
{
    auto result = set_mode(gpioChannel, pin, PI_OUTPUT);
    if (result < 0) {
        log(std::format("Unable to set pin {} for output (error={}).", pin, result));
    }
}

void GPIO::setForInput(unsigned pin)
{
    auto result = set_mode(gpioChannel, pin, PI_INPUT);
    if (result < 0) {
        log(std::format("Unable to set pin {} for input (error={}).", pin, result));
    }
}

void GPIO::setPullUp(unsigned pin)
{
    auto result = set_pull_up_down(gpioChannel, pin, PI_PUD_UP);
    if (result < 0) {
        log(std::format("Unable to set pin {} for pull-up (error={}).", pin, result));
    }
}

void GPIO::setPullDown(unsigned pin)
{
    auto result = set_pull_up_down(gpioChannel, pin, PI_PUD_DOWN);
    if (result < 0) {
        log(std::format("Unable to set pin {} for pull-up (error={}).", pin, result));
    }
}


static std::array<GPIO::GPIOHandler, NumGPIO> gpioRiseHandlers;
static std::array<GPIO::GPIOHandler, NumGPIO> gpioFallHandlers;

static void gpioIRQ([[maybe_unused]] int channel, unsigned gpio, unsigned level, uint32_t tick) {
    if (gpio >= NumGPIO) {
        return;
    }

    if ((level == 0) && gpioFallHandlers[gpio]) {
        gpioFallHandlers[gpio](gpio, tick);
    } else if ((level != 0) && gpioRiseHandlers[gpio]) {
        gpioRiseHandlers[gpio](gpio, tick);
    }
}

static std::array<std::atomic_flag, NumGPIO> callbackRegistered;

static void registerCallback(unsigned pin)
{
    static std::atomic_flag firstRun = ATOMIC_FLAG_INIT;

    if (!std::atomic_flag_test_and_set(&firstRun)) {
        for (unsigned i = 0; i < NumGPIO; i++) {
            callbackRegistered[i].clear();
        }
    }
    if (!std::atomic_flag_test_and_set(&callbackRegistered[pin])) {
        auto result = callback(gpioChannel, pin, EITHER_EDGE, &gpioIRQ);
        if (result < 0) {
            std::cerr << std::format("Failed to register the callback for pin {}.\n", pin);
            throw std::runtime_error("Failed to register callback.");
        }
    }
}


void GPIO::addRiseHandler(unsigned pin, GPIO::GPIOHandler handler)
{
    if (!validPin(pin)) {
        throw new std::runtime_error("Bad pin number.");
    }
    registerCallback(pin);

    gpioRiseHandlers[pin] = handler;
}

void GPIO::addHighHandler([[maybe_unused]] unsigned pin, [[maybe_unused]] GPIO::GPIOHandler handler)
{
    throw new std::runtime_error("High handlers are not implemented.");
}

void GPIO::addFallHandler(unsigned pin, GPIO::GPIOHandler handler)
{
    if (!validPin(pin)) {
        throw new std::runtime_error("Bad pin number.");
    }
    registerCallback(pin);

    gpioFallHandlers[pin] = handler;
}

void GPIO::addLowHandler([[maybe_unused]] unsigned pin, [[maybe_unused]] GPIO::GPIOHandler handler)
{
    throw new std::runtime_error("High handlers are not implemented.");
}

void GPIO::set(unsigned pin, bool value)
{
    openChannel();
    auto result = gpio_write(gpioChannel, pin, value ? 1 : 0);
    if (result < 0) {
        log(std::format("Unable to set pin {} to {} (error={}).", pin, value, result));
    }
}

bool GPIO::get(unsigned pin)
{
    openChannel();
    auto result = gpio_read(gpioChannel, pin);
    if (result < 0) {
        log(std::format("Unable to read pin {} (error={}).", pin, result));
    }
    return result != 0;
}