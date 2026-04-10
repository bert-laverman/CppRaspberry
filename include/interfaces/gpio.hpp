#pragma once
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

#include <bitset>
#include <functional>
#include <stdexcept>

#include <util/verbose-component.hpp>


namespace nl::rakis::raspberrypi::interfaces {

    
static constexpr int NO_PIN{ -1 };

/**
 * the use of the pins. Note that which pins can do what will differ per RPi model.
 */
enum class GPIOMode {
    XIP        = 0x00,      // Memory mapping related (eXecute In Place)
    SPI,                    // SPI communications
    UART,                   // Serial communications
    I2C,                    // I2C communications
    PWM,                    // PWM (Pulse Width Modulation) digital output
    SIO,                    // Software controlled I/O (default)
    PIO0,                   // PIO-0 controlled I/O (Pico only)
    PIO1,                   // PIO-1 controlled I/O (Pico only)
    GPCK,                   // Clock
    USB,                    // USB communications
    Unused     = 0x0f,
    Unavailable,
};


/**
 * Class for GPIO interfaces.
 */
class GPIO : public util::VerboseComponent {
    static constexpr unsigned MaxGPIO{ 64 };
    static std::bitset<MaxGPIO> gpioUsed_;

public:
    GPIO();
    ~GPIO();

    // There can be only one, so no copying or moving.
    GPIO(const GPIO&) = delete;
    GPIO(GPIO&&) = delete;
    GPIO& operator=(const GPIO&) = delete;
    GPIO& operator=(GPIO&&) = delete;


    /**
     * Return true if this GPIO interface is directly connected to the Raspberry Pi.
     */
    bool direct() const noexcept;

    /**
     * Get the number of pins available on this GPIO interface.
     */
    unsigned numPins() const noexcept;

    /**
     * Return true if the given pin is in use. Note that non-existing pins are simply considered to be not in use.
     * 
     * @param pin The pin to check.
     * @throws std::out_of_range if the pin number is out of range.
     */
    bool used(unsigned pin) const {
        if (pin >= MaxGPIO) {
            throw std::out_of_range("Pin number out of range.");
        }
        return gpioUsed_[pin];
    }

    /**
     * Return true if the given pin is available for use. Note that non-existing pins are simply considered to be not available.
     * 
     * @param pin The pin to check.
     * @throws std::out_of_range if the pin number is out of range.
     */
    bool available(unsigned pin) const {
        if (pin >= MaxGPIO) {
            throw std::out_of_range("Pin number out of range.");
        }
        return !gpioUsed_[pin];
    }

    /**
     * Convenience function to check if a given pin number is valid.
     * 
     * @param pin The pin to check.
     */
    bool validPin(unsigned pin) const noexcept { return pin < numPins(); }

    /**
     * Mark a pin as in use, but don't actually do anything with it.
     * 
     * @param pin The pin to claim.
     * @param mode The mode to claim it for.
     * @throws std::out_of_range if the pin number is out of range.
     */
    void claim(unsigned pin, GPIOMode mode = GPIOMode::SIO);

    /**
     * Release the claim on a pin, without any other actions.
     * 
     * @param pin The pin to release.
     * @throws std::out_of_range if the pin number is out of range.
     */
    void release(unsigned pin) { claim(pin, GPIOMode::Unused); }

    /**
     * Claim a pin and indicate what it will be used for. Default mode is Software controlled I/O. Set it to GPIOMode::Unused to release.
     * 
     * @param pin The pin to claim.
     * @param mode The mode to claim it for.
     * @throws std::out_of_range if the pin number is out of range.
     * @throws std::runtime_error if the pin is not available.
     */
    void init(unsigned pin, GPIOMode mode = GPIOMode::SIO);

    /**
     * Release a pin.
     * 
     * @param pin The pin to release.
     * @throws std::out_of_range if the pin number is out of range.
     */
    void deinit(unsigned pin) { init(pin, GPIOMode::Unused); }

    /**
     * Set the given pin as used for output.
     * 
     * @param pin The pin to set for output.
     * @throws std::out_of_range if the pin number is out of range.
     */
    void setForOutput(unsigned pin);

    /**
     * Set the given pin as used for input.
     * 
     * @param pin The pin to set for input.
     * @throws std::out_of_range if the pin number is out of range.
     */
    void setForInput(unsigned pin);

    /**
     * Set the given pin pulled-up.
     * 
     * @param pin The pin to set pulled-up.
     * @throws std::out_of_range if the pin number is out of range.
     */
    void setPullUp(unsigned pin);

    /**
     * Set the given pin pulled-down.
     * 
     * @param pin The pin to set pulled-down.
     * @throws std::out_of_range if the pin number is out of range.
     */
    void setPullDown(unsigned pin);


    /**
     * Handler for GPIO events.
     */
    using GPIOHandler = std::function<void(unsigned pin, uint32_t event)>;

    /**
     * Set an interrupt handler to trigger when the input on the pin changes from low to high.
     * 
     * @param pin The pin to set the handler for.
     * @param handler The handler to call when the event occurs.
     * @throws std::runtime_error if the pin number is out of range.
     */
    void addRiseHandler(unsigned pin, GPIOHandler handler);

    /**
     * Set an interrupt handler to trigger when the input on the pin is high.
     * 
     * @param pin The pin to set the handler for.
     * @param handler The handler to call when the event occurs.
     * @throws std::runtime_error if the pin number is out of range.
     */
    void addHighHandler(unsigned pin, GPIOHandler handler);

    /**
     * Set an interrupt handler to trigger when the input on the pin changes from high to low.
     * 
     * @param pin The pin to set the handler for.
     * @param handler The handler to call when the event occurs.
     * @throws std::runtime_error if the pin number is out of range.
     */
    void addFallHandler(unsigned pin, GPIOHandler handler);

    /**
     * Set an interrupt handler to trigger when the input on the pin is low.
     * 
     * @param pin The pin to set the handler for.
     * @param handler The handler to call when the event occurs.
     * @throws std::runtime_error if the pin number is out of range.
     */
    void addLowHandler(unsigned pin, GPIOHandler handler);

    /**
     * Set the output on the given pin to the given value.
     * 
     * @param pin The pin to set the output for.
     * @param value The value to set the output to.
     * @throws std::runtime_error if the pin number is out of range.
     */
    void set(unsigned pin, bool value);

    /**
     * Get the current input of the given pin.
     * 
     * @param pin The pin to get the input for.
     * @return The current input of the pin.
     * @throws std::out_of_range if the pin number is out of range.
     */
    bool get(unsigned pin);
};

} // namespace nl::rakis::raspberrypi::interfaces