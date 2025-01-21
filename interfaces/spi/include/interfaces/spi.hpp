#pragma once
/*
 * Copyright (c) 2023-2024 by Bert Laverman. All Rights Reserved.
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

#include <span>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>

#include <raspberry-pi.hpp>
#include <util/named-component.hpp>
#include <util/verbose-component.hpp>
#include <devices/spi-device.hpp>


namespace nl::rakis::raspberrypi::interfaces {


static constexpr unsigned speed5MHz = 5'000'000;
static constexpr unsigned speed10MHz = 10'000'000;
static constexpr unsigned speed20MHz = 20'000'000;

/**
 * This class represents a SPI interface, which can have zero or more (daisy-chained) devices connected to it.
 *
 * Note that the same physical SPI connection can be reused by different chains of devices by using separate CS (Chip Select) lines.
 */
template <class SpiClass>
class SPI : public util::VerboseComponent, public util::NamedComponent
{
public:
    static constexpr int NO_PIN{ -1 };

private:
    int csPin_{ NO_PIN };
    int sclkPin_{ NO_PIN };
    int mosiPin_{ NO_PIN };
    int misoPin_{ NO_PIN };

    unsigned speed_{ speed5MHz };
    bool selected_{ false };

public:
    SPI() = default;
    SPI(int csPin, int sclkPin, int mosiPin) : csPin_{csPin}, sclkPin_{sclkPin}, mosiPin_{mosiPin} {}
    SPI(int csPin, int sclkPin, int mosiPin, int misoPin) : csPin_{csPin}, sclkPin_{sclkPin}, mosiPin_{mosiPin}, misoPin_{misoPin} {}

    ~SPI() = default;
    SPI(SPI &&) = default;
    SPI &operator=(SPI &&) = default;

    /**
     * No copying of a SPI interface is allowed, because you'd lose sight of who owns it.
     */
    SPI(SPI const &) = delete;
    SPI &operator=(SPI const &) = delete;

    /**
     * Set the GPIO pin to be used for the Chip-Select (sometimes Chip Enable or CE) line. Setting the value forces a close.
     */
    void csPin(unsigned pin) { close(); csPin_ = pin; }

    /**
     * Return the GPIO pin used for the Chip-Select (sometimes Chip Enable or CE) line.
     */
    int csPin() const noexcept { return csPin_; }

    /**
     * Set the GPIO pin to be used for the clock signal. Setting the value forces a close.
     */
    void sclkPin(unsigned pin) { close(); sclkPin_ = pin; }

    /**
     * Return the GPIO pin used for the clock signal.
     */
    int sclkPin() const noexcept { return sclkPin_; }

    /**
     * Set the GPIO pin used for the Master-Out-Slave-In (sometimes Data-Out or TX) line. Setting the value forces a close.
     */
    void mosiPin(unsigned pin) { close(); mosiPin_ = pin; }

    /**
     * Return the GPIO pin used for the Master-Out-Slave-In (sometimes Data-Out or TX) line.
     */
    int mosiPin() const noexcept { return mosiPin_; }

    /**
     * Set the GPIO pin used for the Master-In-Slave-Out (sometimes Data-In or RX) line. Setting the value forces a close.
     */
    void misoPin(unsigned pin) { close(); misoPin_ = pin; }

    /**
     * Return the GPIO pin used for the Master-In-Slave-Out (sometimes Data-In or RX) line.
     */
    int misoPin() const noexcept { return misoPin_; }

    /**
     * Set if the connection is a 4-pin SPI interface, in which case a  full-duplex connection is used. Setting the value forces a close.
     */
    void set3Pin() { misoPin(NO_PIN); }

    /**
     * Indicate if this is a 4-pin SPI interface, in which case a full-duplex connection is used.
     */
    bool is4Pin() const noexcept { return misoPin_ == NO_PIN; }

    /**
     * Set the BAUD rate for this connection. Setting the value forces a close.
     */
    void baudRate(unsigned baudRate) { close(); speed_ = baudRate; }

    /**
     * Return the BAUD rate used for this interface.
     */
    unsigned baudRate() const noexcept { return speed_; }

    /**
     * Return if this interface has been correctly set up and can be used.
     */
    operator bool() const noexcept { return static_cast<SpiClass*>(this)->initialized(); }

    /**
     * Initialize the connection, assuring the device is configured correctly.
     */
    void open() { static_cast<SpiClass*>(this)->doOpen(); }

    /**
     * De-initialize the connection, releasing the device (pins) for other use.
     */
    void close() { static_cast<SpiClass*>(this)->doClose(); }

    /**
     * Reset the connection.
     */
    void reset() { close(); open(); }

    /**
     * Return if this interface is currently active, i.e. is the CS line is selected.
     */
    bool selected() { return selected_; }

    /**
     * Enable the CS line for this SPI chain. (active low)
     */
    void select() { RaspberryPi::gpio().set(csPin(), false); selected_ = true; }

    /**
     * Release the CS line for this SPI chain.
     */
    void deselect() { RaspberryPi::gpio().set(csPin(), true); selected_ = false; }

    /**
     * Write the given set of bytes.
     */
    void write(const std::span<uint8_t> value) { static_cast<SpiClass*>(this)->doWrite(value); }

};

} // namespace nl::rakis::raspberrypi::interfaces