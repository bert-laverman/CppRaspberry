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


#include <functional>

#include <util/verbose-component.hpp>


namespace nl::rakis::raspberrypi::interfaces {

/**
 * @brief the use of the pins. Note that which pins can do what will differ per RPi model.
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
 * @brief Class for GPIO interfaces.
 */
class GPIO : public util::VerboseComponent {
public:
    GPIO();
    ~GPIO();

    // There can be only one, so no copying or moving.
    GPIO(const GPIO&) = delete;
    GPIO(GPIO&&) = delete;
    GPIO& operator=(const GPIO&) = delete;
    GPIO& operator=(GPIO&&) = delete;


    /**
     * @brief Return true if this GPIO interface is directly connected to the Raspberry Pi.
     */
    bool direct() const noexcept;

    /**
     * @brief Get the number of pins available on this GPIO interface.
     */
    unsigned numPins() const noexcept;

    /**
     * @brief Return true if the given pin is in use. Note that non-existing pins are simply considered to be not in use.
     */
    bool used(unsigned pin) const noexcept;

    /**
     * @brief Return true if the given pin is available for use. Note that non-existing pins are simply considered to be not available.
     */
    bool available(unsigned pin) const noexcept;

    /**
     * @brief Convenience function to check if a given pin is valid.
     */
    bool validPin(unsigned pin) const noexcept;

    /**
     * @brief Mark a pin as in use, but don't actually do anything with it.
     */
    void claim(unsigned pin, GPIOMode mode = GPIOMode::SIO);

    /**
     * @brief Release the claim on a pin, without any other actions.
     */
    void release(unsigned pin) { claim(pin, GPIOMode::Unused); }

    /**
     * @brief Claim a pin and indicate what is will be used for. Default mode is Software controlled I/O. Set it to GPIOMode::Unused to release.
     */
    void init(unsigned pin, GPIOMode mode = GPIOMode::SIO);

    /**
     * @brief Release a pin.
     */
    void deinit(unsigned pin) { init(pin, GPIOMode::Unused); }

    /**
     * @brief Set the given pin as used for output.
     */
    void setForOutput(unsigned pin);

    /**
     * @brief Set the given pin as used for input.
     */
    void setForInput(unsigned pin);

    /**
     * @brief Set the given pin pulled-up.
     */
    void setPullUp(unsigned pin);

    /**
     * @brief Set the given pin pulled-down.
     */
    void setPullDown(unsigned pin);


    using GPIOHandler = std::function<void(unsigned pin, uint32_t event)>;

    /**
     * @brief Set an interrup handler to trigger when the input on the pin changes from low to high.
     */
    void addRiseHandler(unsigned pin, GPIOHandler handler);

    /**
     * @brief Set an interrup handler to trigger when the input on the pin is high.
     */
    void addHighHandler(unsigned pin, GPIOHandler handler);

    /**
     * @brief Set an interrup handler to trigger when the input on the pin changes from high to low.
     */
    void addFallHandler(unsigned pin, GPIOHandler handler);

    /**
     * @brief Set an interrup handler to trigger when the input on the pin is low.
     */
    void addLowHandler(unsigned pin, GPIOHandler handler);

    /**
     * @brief Set the output on the given pin to the given value.
     */
    void set(unsigned pin, bool value);

    /**
     * @brief Get the current input of the given pin.
     */
    bool get(unsigned pin);
};

} // namespace nl::rakis::raspberrypi::interfaces