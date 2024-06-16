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


#include <hardware/gpio.h>

#include <array>
#include <functional>
#include <stdexcept>

#include <interfaces/gpio.hpp>


using namespace nl::rakis::raspberrypi::interfaces;


static constexpr unsigned int NumGPIO{ 30 };

static std::array<GPIOMode, NumGPIO> mode_{{
    // Left side, as viewed from above
    GPIOMode::Unused,       // GPIO  0: Pin  1, default UART0 TX
    GPIOMode::Unused,       // GPIO  1: Pin  2, default UART0 RX
                            //          Pin  3, GND
    GPIOMode::Unused,       // GPIO  2: Pin  4
    GPIOMode::Unused,       // GPIO  3: Pin  5
    GPIOMode::Unused,       // GPIO  4: Pin  6, default I2C0 SDA
    GPIOMode::Unused,       // GPIO  5: Pin  7, default I2C0 SCL
                            //          Pin  8, GND
    GPIOMode::Unused,       // GPIO  6: Pin  9
    GPIOMode::Unused,       // GPIO  7: Pin 10
    GPIOMode::Unused,       // GPIO  8: Pin 11
    GPIOMode::Unused,       // GPIO  9: Pin 12
                            //          Pin 13, GND
    GPIOMode::Unused,       // GPIO 10: Pin 14
    GPIOMode::Unused,       // GPIO 11: Pin 15
    GPIOMode::Unused,       // GPIO 12: Pin 16
    GPIOMode::Unused,       // GPIO 13: Pin 17
                            //          Pin 18, GND
    GPIOMode::Unused,       // GPIO 14: Pin 19
    GPIOMode::Unused,       // GPIO 15: Pin 20

    // Right side, as viewed from above
    GPIOMode::Unused,       // GPIO 16, Pin 21, default SPI0 MISO/RX
    GPIOMode::Unused,       // GPIO 17, Pin 22, default SPI0 CS
                            //          Pin 23, GND
    GPIOMode::Unused,       // GPIO 18, Pin 24, default SPI0 SCK/CLK
    GPIOMode::Unused,       // GPIO 19, Pin 25, default SPI0 MOSI/TX
    GPIOMode::Unused,       // GPIO 20, Pin 26
    GPIOMode::Unused,       // GPIO 21, Pin 27
                            //          Pin 28, GND
    GPIOMode::Unused,       // GPIO 22, Pin 29
                            //          Pin 30, RUN
    GPIOMode::Unavailable,  // GPIO 23
    GPIOMode::Unavailable,  // GPIO 24
    GPIOMode::Unused,       // GPIO 25,         internal Led
    GPIOMode::Unused,       // GPIO 26, Pin 31, default ADC0
    GPIOMode::Unused,       // GPIO 27, Pin 32, default ADC1
                            //          Pin 33, GND, ADC GND
    GPIOMode::Unused,       // GPIO 28, Pin 34, default ADC2
                            //          Pin 35, ADC VREF
                            //          Pin 36, 3V3 (OUT)
                            //          Pin 37, 3V3_EN
                            //          Pin 38, GND
                            //          Pin 39, VSYS (5V)
                            //          Pin 40, VBUS (5V)
    GPIOMode::Unavailable   // GPIO 29
}};


inline static GPIOMode mode(unsigned pin) {
    return (pin >= NumGPIO) ? GPIOMode::Unavailable : mode_[pin];
}

GPIO::GPIO()
{
}

GPIO::~GPIO()
{
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

void GPIO::init(unsigned pin, GPIOMode m)
{
    if (m == GPIOMode::Unused) {
        if (used(pin)) {
            if (verbose()) {
                log() << "Releasing pin " << pin << ".\n";
            }
            mode_[pin] = m;
            gpio_deinit(pin);
        } else if (verbose()) {
            log() << "Releasing pin " << pin << ", but it already was unused.\n";
        }
        return;
    }
    if (!available(pin)) {
        if (verbose()) {
            log() << "Pin " << pin << " is not available.\n";
        }
        throw new std::runtime_error("Pin not available.");
    }
    if (verbose()) {
        log() << "Initializing pin " << pin << ".\n";
    }
    mode_[pin] = m;
    gpio_init(pin);
    gpio_set_function(pin, static_cast<gpio_function>(m));
}

void GPIO::setForOutput(unsigned pin)
{
    gpio_set_dir(pin, true);
}

void GPIO::setForInput(unsigned pin)
{
    gpio_set_dir(pin, false);
}

void GPIO::setPullUp(unsigned pin)
{
    gpio_pull_up(pin);
}

void GPIO::setPullDown(unsigned pin)
{
    gpio_pull_down(pin);
}


static std::array<GPIO::GPIOHandler, NumGPIO> gpioHighHandlers;
static std::array<GPIO::GPIOHandler, NumGPIO> gpioLowHandlers;
static std::array<GPIO::GPIOHandler, NumGPIO> gpioRiseHandlers;
static std::array<GPIO::GPIOHandler, NumGPIO> gpioFallHandlers;


inline static uint32_t irqMask(unsigned pin) {
    uint32_t mask{0};

    if (gpioHighHandlers[pin]) mask |= GPIO_IRQ_LEVEL_HIGH;
    if (gpioLowHandlers[pin]) mask |= GPIO_IRQ_LEVEL_LOW;
    if (gpioRiseHandlers[pin]) mask |= GPIO_IRQ_EDGE_RISE;
    if (gpioFallHandlers[pin]) mask |= GPIO_IRQ_EDGE_FALL;

    return mask;
}

static void gpioIRQ(uint gpio, uint32_t events) {
    if (gpio >= NumGPIO) {
        return;
    }

    if (((events & GPIO_IRQ_LEVEL_LOW) != 0) && gpioLowHandlers[gpio]) {
        gpioLowHandlers[gpio](gpio, events);
    } else if (((events & GPIO_IRQ_LEVEL_HIGH) != 0) && gpioHighHandlers[gpio]) {
        gpioHighHandlers[gpio](gpio, events);
    } else if (((events & GPIO_IRQ_EDGE_FALL) != 0) && gpioFallHandlers[gpio]) {
        gpioFallHandlers[gpio](gpio, events);
    } else if (((events & GPIO_IRQ_EDGE_RISE) != 0) && gpioRiseHandlers[gpio]) {
        gpioRiseHandlers[gpio](gpio, events);
    }
}

void GPIO::addRiseHandler(unsigned pin, GPIO::GPIOHandler handler)
{
    if (!validPin(pin)) {
        throw new std::runtime_error("Bad pin number.");
    }
    gpioRiseHandlers[pin] = handler;

    gpio_set_irq_enabled_with_callback(pin, irqMask(pin), true, &gpioIRQ);
}

void GPIO::addHighHandler(unsigned pin, GPIO::GPIOHandler handler)
{
    if (!validPin(pin)) {
        throw new std::runtime_error("Bad pin number.");
    }
    gpioHighHandlers[pin] = handler;

    gpio_set_irq_enabled_with_callback(pin, irqMask(pin), true, &gpioIRQ);
}

void GPIO::addFallHandler(unsigned pin, GPIO::GPIOHandler handler)
{
    if (!validPin(pin)) {
        throw new std::runtime_error("Bad pin number.");
    }
    gpioFallHandlers[pin] = handler;

    gpio_set_irq_enabled_with_callback(pin, irqMask(pin), true, &gpioIRQ);
}

void GPIO::addLowHandler(unsigned pin, GPIO::GPIOHandler handler)
{
    if (!validPin(pin)) {
        throw new std::runtime_error("Bad pin number.");
    }
    gpioLowHandlers[pin] = handler;

    gpio_set_irq_enabled_with_callback(pin, irqMask(pin), true, &gpioIRQ);
}

void GPIO::set(unsigned pin, bool value)
{
    gpio_put(pin, value);
}

bool GPIO::get(unsigned pin)
{
    return gpio_get(pin);
}