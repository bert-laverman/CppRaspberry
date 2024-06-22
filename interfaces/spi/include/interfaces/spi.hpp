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


/**
 * @brief This class represents a SPI interface, which can have zero or more (daisy-chained) devices connected to it.
 *
 * Note that the same physical SPI connection can be reused by different chains of devices by using separate CS (Chip Select) lines.
 */
class SPI : public util::VerboseComponent, public util::NamedComponent, public std::enable_shared_from_this<SPI>
{
    bool is4Pin_;
    unsigned csPin_;
    unsigned sclkPin_;
    unsigned mosiPin_;
    unsigned misoPin_;

    unsigned int baudRate_{ 5*1000*1000 };

    std::shared_ptr<devices::SPIDevice> device_;

public:
    SPI();

    SPI(SPI const &) = delete;
    SPI(SPI &&) = default;
    SPI &operator=(SPI const &) = delete;
    SPI &operator=(SPI &&) = default;

    virtual ~SPI();

    /**
     * @brief Return the device connected to this interface. If there are multiple devices daisy-chained, they are assumed to be all of the same type.
     */
    std::shared_ptr<devices::SPIDevice> device() { return device_; }

    /**
     * @brief Set the device connected to this interface. If there are multiple devices daisy-chained, they are assumed to be all of the same type.
     */
    void device(std::shared_ptr<devices::SPIDevice> dev) { device_ = dev; dev->interface(shared_from_this()); }

    /**
     * @brief Set the GPIO pin to be used for the Chip-Select (sometimes Chip Enable or CE) line. Setting the value forces a close.
     */
    void csPin(unsigned pin) { close(); csPin_ = pin; }

    /**
     * @brief Return the GPIO pin used for the Chip-Select (sometimes Chip Enable or CE) line.
     */
    unsigned csPin() const noexcept { return csPin_; }

    /**
     * @brief Set the GPIO pin to be used for the clock signal. Setting the value forces a close.
     */
    void sclkPin(unsigned pin) { close(); sclkPin_ = pin; }

    /**
     * @brief Return the GPIO pin used for the clock signal.
     */
    unsigned sclkPin() const noexcept { return sclkPin_; }

    /**
     * @brief Set the GPIO pin used for the Master-Out-Slave-In (sometimes Data-Out or TX) line. Setting the value forces a close.
     */
    void mosiPin(unsigned pin) { close(); mosiPin_ = pin; }

    /**
     * @brief Return the GPIO pin used for the Master-Out-Slave-In (sometimes Data-Out or TX) line.
     */
    unsigned mosiPin() const noexcept { return mosiPin_; }

    /**
     * @brief Set if the connection is a 4-pin SPI interface, in which case a  full-duplex connection is used. Setting the value forces a close.
     */
    void is4Pin(bool fullDuplex) { close(); is4Pin_ = fullDuplex; }
    /**
     * @brief Indicate if this is a 4-pin SPI interface, in which case a full-duplex connection is used.
     */
    bool is4Pin() const noexcept { return is4Pin_; }

    /**
     * @brief Set the GPIO pin used for the Master-In-Slave-Out (sometimes Data-In or RX) line. Setting the value forces a close.
     */
    void misoPin(unsigned pin) { close(); misoPin_ = pin; }

    /**
     * @brief Return the GPIO pin used for the Master-In-Slave-Out (sometimes Data-In or RX) line.
     */
    unsigned misoPin() const noexcept { return misoPin_; }

    /**
     * @brief Set the BAUD rate for this connection. Setting the value forces a close.
     */
    void baudRate(unsigned baudRate) { close(); baudRate_ = baudRate; }

    /**
     * @brief Return the BAUD rate used for this interface.
     */
    unsigned baudRate() const noexcept { return baudRate_; }

    /**
     * @brief Return if this interface has been correctly set up and can be used.
     */
    virtual operator bool() const noexcept = 0;

    /**
     * @brief Initialize the connection, assuring the device is configured correctly.
     */
    virtual void open() = 0;

    /**
     * @brief De-initialize the connection, releasing the device (pins) for other use.
     */
    virtual void close() = 0;

    /**
     * @brief Reset the connection.
     */
    void reset() { close(); open(); }

    /**
     * @brief Return if this interface is currently active, i.e. is the CS line is selected.
     */
    virtual bool selected() const noexcept;

    /**
     * @brief Enable the CS line for this SPI chain.
     */
    virtual void select() = 0;

    /**
     * @brief Release the CS line for this SPI chain.
     */
    virtual void deselect() = 0;

    /**
     * @brief Write the given set of bytes.
     */
    virtual void write(std::span<uint8_t> value) =0;

};

} // namespace nl::rakis::raspberrypi::interfaces