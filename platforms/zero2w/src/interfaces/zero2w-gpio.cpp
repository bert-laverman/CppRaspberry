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

#include <iostream>

#include <interfaces/gpio.hpp>


using namespace nl::rakis::raspberrypi::interfaces;


/**
 * The actual number of GPIO pins available on a Pico.
 */
static constexpr unsigned NumGPIO{ 28 };


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


/**
 * Since we're using the pigpiod library, we need to open a channel to it.
 */
static int gpioChannel{ -1 };


/**
 * Open a channel to pigpiod. This function is idempotent.
 * 
 * @throws std::runtime_error if the channel cannot be opened.
 */
static void openChannel()
{
    if (gpioChannel < 0) {
        if ((gpioChannel = pigpio_start(nullptr, nullptr)) < 0) {
            std::cerr << "Failed to open the pigpiod channel.\n";
            throw std::runtime_error("Failed to open channel to pigpiod.\n");
        }
    }
}


/**
 * Close the channel to pigpiod. If closing the connection fails, a message can be logged, but no exception is thrown.
 * 
 * @param verbose If true, log a message to stderr.
 */
static void closeChannel(bool verbose =false)
{
    if (gpioChannel >= 0) {
        if (verbose) {
            std::cerr << "Closing channel to pigpiod for GPIO.\n";
        }
        pigpio_stop(gpioChannel);
        gpioChannel = -1;
    }
}


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
        }
    }
    closeChannel();
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
    if (gpioChannel < 0) {
        log("Opening channel to pigpiod for GPIO.");
        openChannel();
    }

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
        throw std::runtime_error("Pin not available.");
    }
    log(std::format("Claiming pin {}.", pin));

    mode(pin) = m;
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
    if (gpioChannel < 0) {
        log("Opening channel to pigpiod for GPIO.");
        openChannel();
    }

    auto result = set_mode(gpioChannel, pin, PI_OUTPUT);
    if (result < 0) {
        log(std::format("Unable to set pin {} for output (error={}).", pin, result));
    }
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
    if (gpioChannel < 0) {
        log("Opening channel to pigpiod for GPIO.");
        openChannel();
    }

    auto result = set_mode(gpioChannel, pin, PI_INPUT);
    if (result < 0) {
        log(std::format("Unable to set pin {} for input (error={}).", pin, result));
    }
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
    if (gpioChannel < 0) {
        log("Opening channel to pigpiod for GPIO.");
        openChannel();
    }

    auto result = set_pull_up_down(gpioChannel, pin, PI_PUD_UP);
    if (result < 0) {
        log(std::format("Unable to set pin {} for pull-up (error={}).", pin, result));
    }
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
    if (gpioChannel < 0) {
        log("Opening channel to pigpiod for GPIO.");
        openChannel();
    }

    auto result = set_pull_up_down(gpioChannel, pin, PI_PUD_DOWN);
    if (result < 0) {
        log(std::format("Unable to set pin {} for pull-up (error={}).", pin, result));
    }
}


static std::array<GPIO::GPIOHandler, NumGPIO> gpioRiseHandlers;
static std::array<GPIO::GPIOHandler, NumGPIO> gpioFallHandlers;


/**
 * The callback function for GPIO interrupts. An out of range pin number should not occur, but if it does, it is ignored.
 * 
 * @param channel The channel number.
 * @param gpio The GPIO pin number.
 * @param level The level of the signal.
 * @param tick The time of the event.
 */
static void gpioIRQ([[maybe_unused]] int channel, unsigned gpio, unsigned level, uint32_t tick) {
    if (gpio >= NumGPIO) { return; }

    if ((level == 0) && gpioFallHandlers[gpio]) {
        gpioFallHandlers[gpio](gpio, tick);
    } else if ((level != 0) && gpioRiseHandlers[gpio]) {
        gpioRiseHandlers[gpio](gpio, tick);
    }
}


static std::array<std::atomic_flag, NumGPIO> callbackRegistered;


/**
 * Register the callback for a pin.
 * 
 * @param pin The pin number.
 */
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


/**
 * Set an interrupt handler to trigger when the input on the pin changes from low to high.
 * 
 * @param pin The pin to set the handler for.
 * @param handler The handler to call when the event occurs.
 * @throws std::runtime_error if the pin number is out of range.
 */
void GPIO::addRiseHandler(unsigned pin, GPIO::GPIOHandler handler)
{
    if (!validPin(pin)) {
        throw std::runtime_error("Bad pin number.");
    }
    if (gpioChannel < 0) {
        log("Opening channel to pigpiod for GPIO.");
        openChannel();
    }

    registerCallback(pin);

    gpioRiseHandlers[pin] = handler;
}

/**
 * Set an interrupt handler to trigger when the input on the pin is high.
 * 
 * Always throws an exception, as this is not implemented.
 * 
 * @param pin The pin to set the handler for.
 * @param handler The handler to call when the event occurs.
 * @throws std::runtime_error if the pin number is out of range.
 */
void GPIO::addHighHandler([[maybe_unused]] unsigned pin, [[maybe_unused]] GPIO::GPIOHandler handler)
{
    throw std::runtime_error("High handlers are not implemented.");
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
    if (!validPin(pin)) {
        throw std::runtime_error("Bad pin number.");
    }
    if (gpioChannel < 0) {
        log("Opening channel to pigpiod for GPIO.");
        openChannel();
    }

    registerCallback(pin);

    gpioFallHandlers[pin] = handler;
}


/**
 * Set an interrupt handler to trigger when the input on the pin is low.
 * 
 * Always throws an exception, as this is not implemented.
 * 
 * @param pin The pin to set the handler for.
 * @param handler The handler to call when the event occurs.
 * @throws std::runtime_error if the pin number is out of range.
 */
void GPIO::addLowHandler([[maybe_unused]] unsigned pin, [[maybe_unused]] GPIO::GPIOHandler handler)
{
    throw std::runtime_error("Low handlers are not implemented.");
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
    if (!validPin(pin)) {
        throw std::out_of_range("Pin number out of range.");
    }
    if (gpioChannel < 0) {
        log("Opening channel to pigpiod for GPIO.");
        openChannel();
    }

    log(std::format("Setting pin {} to {}.", pin, value ? 1 : 0));
    auto result = gpio_write(gpioChannel, pin, value ? 1 : 0);
    if (result < 0) {
        log(std::format("Unable to set pin {} to {} (error={}).", pin, value, result));
    }
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
    if (!validPin(pin)) {
        throw std::out_of_range("Pin number out of range.");
    }
    if (gpioChannel < 0) {
        log("Opening channel to pigpiod for GPIO.");
        openChannel();
    }

    auto result = gpio_read(gpioChannel, pin);
    if (result < 0) {
        log(std::format("Unable to read pin {} (error={}).", pin, result));
    }
    return result != 0;
}