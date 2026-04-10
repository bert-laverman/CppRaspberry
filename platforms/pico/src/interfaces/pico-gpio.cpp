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
#include <format>
#include <functional>
#include <stdexcept>

#include <interfaces/gpio.hpp>


using namespace nl::rakis::raspberrypi::interfaces;


/**
 * The actual number of GPIO pins available on a Pico.
 */
static constexpr unsigned NumGPIO{ 30 };


/**
 * Whether the pin is used or not.
 */
std::bitset<GPIO::MaxGPIO> GPIO::gpioUsed_;


/**
 * The mode of each pin.
 * 
 * TODO Deprecate this?
 */
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


/**
 * Initialize the GPIO interface.
 */
GPIO::GPIO()
{
    gpioUsed_.reset();

    // Mark all unavailable pins as used.
    for (unsigned i = 0; i < NumGPIO; i++) {
        if (mode_[i] == GPIOMode::Unavailable) {
            gpioUsed_.set(i);
        }
    }
}


/**
 * Release all pins.
 */
GPIO::~GPIO()
{
    // Release all pins.
    for (unsigned i = 0; i < NumGPIO; i++) {
        if (used(i)) {
            release(i);
            gpio_deinit(i);
        }
    }
}


/**
 * Return the mode of a pin.
 *
 * @param pin The pin number.
 * @return The mode of the pin.
 * @throws std::out_of_range if the pin number is out of range.
 */
inline static GPIOMode& mode(unsigned pin) {
    if (pin >= NumGPIO) {
        throw std::out_of_range("Pin number out of range.");
    }
    return mode_[pin];
}


/**
 * Indicate if this interface is directly connected to the board, rather than through a GPIO expander.
 */
bool GPIO::direct() const noexcept
{
    return true;
}


/**
 * Return the number of pins available.
 *
 * @return The number of pins available.
 */
unsigned GPIO::numPins() const noexcept
{
    return NumGPIO;
}


/**
 * Set a pin's availability or use
 * @param pin The pin number.
 * @return true if the pin is available, false if it is in use.
 * @throws std::out_of_range if the pin number is out of range.
 */
void GPIO::claim(unsigned pin, GPIOMode m)
{
    mode(pin) = m;
    gpioUsed_.set(pin, m != GPIOMode::Unused);
}


/**
 * Initialize a pin for use.
 * 
 * @param pin The pin number.
 * @param m The mode to set the pin to.
 * @throws std::out_of_range if the pin number is out of range.
 * @throws std::runtime_error if the pin is not available.
 */
void GPIO::init(unsigned pin, GPIOMode m)
{
    if (m == GPIOMode::Unused) {
        if (used(pin)) {
            log(std::format("Releasing pin {}.", pin));

            release(pin);
            gpio_deinit(pin);
        } else {
            log(std::format("Releasing pin {} but it was already released.\n", pin));
        }
        return;
    }
    if (!available(pin)) {
        log(std::format("Pin {} is not available.\n", pin));

        throw std::runtime_error("Pin not available.");
    }
    if (verbose()) {
        log(std::format("Initializing pin {} for {} use. (mode {})",
            pin,
            (m == GPIOMode::SPI ? "SPI" : (m == GPIOMode::I2C ? "I2C" : "GPIO")),
            static_cast<int>(m)));
    }
    claim(pin, m);
    if (m == GPIOMode::SIO) {
        gpio_init(pin);
    } else {
        gpio_set_function(pin, static_cast<gpio_function_t>(m));
    }
}


/**
 * Set the given pin as used for output.
 * 
 * @param pin The pin to set for output.
 * @throws std::out_of_range if the pin number is out of range.
 */
void GPIO::setForOutput(unsigned pin)
{
    if (pin >= NumGPIO) {
        throw std::out_of_range("Pin number out of range.");
    }
    gpio_set_dir(pin, true);
}


/**
 * Set the given pin as used for input.
 * 
 * @param pin The pin to set for input.
 * @throws std::out_of_range if the pin number is out of range.
 */
void GPIO::setForInput(unsigned pin)
{
    if (pin >= NumGPIO) {
        throw std::out_of_range("Pin number out of range.");
    }
    gpio_set_dir(pin, false);
}


/**
 * Set the given pin pulled-up.
 * 
 * @param pin The pin to set pulled-up.
 * @throws std::out_of_range if the pin number is out of range.
 */
void GPIO::setPullUp(unsigned pin)
{
    if (pin >= NumGPIO) {
        throw std::out_of_range("Pin number out of range.");
    }
    gpio_pull_up(pin);
}


/**
 * Set the given pin pulled-down.
 * 
 * @param pin The pin to set pulled-down.
 * @throws std::out_of_range if the pin number is out of range.
 */
void GPIO::setPullDown(unsigned pin)
{
    if (pin >= NumGPIO) {
        throw std::out_of_range("Pin number out of range.");
    }
    gpio_pull_down(pin);
}


static std::array<GPIO::GPIOHandler, NumGPIO> gpioHighHandlers;
static std::array<GPIO::GPIOHandler, NumGPIO> gpioLowHandlers;
static std::array<GPIO::GPIOHandler, NumGPIO> gpioRiseHandlers;
static std::array<GPIO::GPIOHandler, NumGPIO> gpioFallHandlers;


/**
 * Compute the IRQ mask for a pin.
 * 
 * @param pin The pin number.
 * @return The IRQ mask for the pin.
 * @throws std::out_of_range if the pin number is out of range.
 */
inline static uint32_t irqMask(unsigned pin) {
    if (pin >= NumGPIO) {
        throw std::out_of_range("Pin number out of range.");
    }
    uint32_t mask{0};

    if (gpioHighHandlers[pin]) mask |= GPIO_IRQ_LEVEL_HIGH;
    if (gpioLowHandlers[pin]) mask |= GPIO_IRQ_LEVEL_LOW;
    if (gpioRiseHandlers[pin]) mask |= GPIO_IRQ_EDGE_RISE;
    if (gpioFallHandlers[pin]) mask |= GPIO_IRQ_EDGE_FALL;

    return mask;
}


/**
 * Handle GPIO interrupts. An out of range pin number should not occur, but if it does, it is ignored.
 * 
 * @param pin The pin number.
 * @param events The events that occurred.
 */
static void gpioIRQ(uint pin, uint32_t events) noexcept {
    if (pin >= NumGPIO) { return; }

    if (((events & GPIO_IRQ_LEVEL_LOW) != 0) && gpioLowHandlers[pin]) {
        gpioLowHandlers[pin](pin, events);
    } else if (((events & GPIO_IRQ_LEVEL_HIGH) != 0) && gpioHighHandlers[pin]) {
        gpioHighHandlers[pin](pin, events);
    } else if (((events & GPIO_IRQ_EDGE_FALL) != 0) && gpioFallHandlers[pin]) {
        gpioFallHandlers[pin](pin, events);
    } else if (((events & GPIO_IRQ_EDGE_RISE) != 0) && gpioRiseHandlers[pin]) {
        gpioRiseHandlers[pin](pin, events);
    }
}


/**
 * Set an interrupt handler to trigger when the input on the pin changes from low to high.
 * 
 * @param pin The pin to set the handler for.
 * @param handler The handler to call when the event occurs.
 * @throws std::runtime_error if the pin number is out of range.
 */
void GPIO::addRiseHandler(unsigned pin, GPIO::GPIOHandler handler)
{
    if (pin >= NumGPIO) {
        throw std::runtime_error("Pin number out of range.");
    }
    gpioRiseHandlers[pin] = handler;

    gpio_set_irq_enabled_with_callback(pin, irqMask(pin), true, &gpioIRQ);
}


/**
 * Set an interrupt handler to trigger when the input on the pin is high.
 * 
 * @param pin The pin to set the handler for.
 * @param handler The handler to call when the event occurs.
 * @throws std::runtime_error if the pin number is out of range.
 */
void GPIO::addHighHandler(unsigned pin, GPIO::GPIOHandler handler)
{
    if (pin >= NumGPIO) {
        throw std::runtime_error("Pin number out of range.");
    }
    gpioHighHandlers[pin] = handler;

    gpio_set_irq_enabled_with_callback(pin, irqMask(pin), true, &gpioIRQ);
}


/**
 * Set an interrupt handler to trigger when the input on the pin changes from high to low.
 * 
 * @param pin The pin to set the handler for.
 * @param handler The handler to call when the event occurs.
 * @throws std::runtime_error if the pin number is out of range.
 */
void GPIO::addFallHandler(unsigned pin, GPIO::GPIOHandler handler)
{
    if (pin >= NumGPIO) {
        throw std::runtime_error("Pin number out of range.");
    }
    gpioFallHandlers[pin] = handler;

    gpio_set_irq_enabled_with_callback(pin, irqMask(pin), true, &gpioIRQ);
}


/**
 * Set an interrupt handler to trigger when the input on the pin is low.
 * 
 * @param pin The pin to set the handler for.
 * @param handler The handler to call when the event occurs.
 * @throws std::runtime_error if the pin number is out of range.
 */
void GPIO::addLowHandler(unsigned pin, GPIO::GPIOHandler handler)
{
    if (pin >= NumGPIO) {
        throw std::out_of_range("Pin number out of range.");
    }
    gpioLowHandlers[pin] = handler;

    gpio_set_irq_enabled_with_callback(pin, irqMask(pin), true, &gpioIRQ);
}


/**
 * Set the value of a pin.
 * 
 * @param pin The pin number.
 * @param value The value to set the pin to.
 * @throws std::out_of_range if the pin number is out of range.
 */
void GPIO::set(unsigned pin, bool value)
{
    if (pin >= NumGPIO) {
        throw std::out_of_range("Pin number out of range.");
    }
    gpio_put(pin, value);
}


/**
 * Get the value of a pin.
 * 
 * @param pin The pin number.
 * @return The value of the pin.
 * @throws std::out_of_range if the pin number is out of range.
 */
bool GPIO::get(unsigned pin)
{
    if (pin >= NumGPIO) {
        throw std::out_of_range("Pin number out of range.");
    }
    return gpio_get(pin);
}