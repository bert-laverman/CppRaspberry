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


#if !defined(HAVE_SPI)
#error "SPI support is required for the MAX7219 driver"
#endif

#include <cstdint>

#include <span>
#include <array>
#include <vector>

#include <devices/spi-device.hpp>

namespace nl::rakis::raspberrypi::devices {

constexpr static const unsigned MAX7219_DIGITS = 8;

struct MAX7219Module {
    bool enabled{ false };

    uint8_t brightness{ 0 };
    uint8_t scanLimit{ 7 };
    uint8_t decodeMode{ 255 };

    bool hasValue{ false };
    int32_t value{ 0 };
    std::array<uint8_t, MAX7219_DIGITS> buffer{ 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f };
};

/**
 * Represents a MAX7219 device or a (daisy-chained) set of MAX7219 devices.
 * 
 * This class provides an interface to control a MAX7219 device using SPI communication.
 * It allows setting brightness, scan limit, decode mode, and other parameters of the device.
 * It also provides methods to display numbers and clear the display.
 */
template <class MaxClass>
class MAX7219 {
    std::vector<MAX7219Module> buffer_;

    bool dirtyBrightness_{ false };
    bool dirtyScanLimit_{ false };
    bool dirtyDecodeMode_{ false };
    bool dirtyBuffer_{ false };

    bool padding_{ false };
    bool writeImmediately_{ true };

protected:
    std::vector<MAX7219Module>& buffer() { return buffer_; }

    void storeBrightness(unsigned module, uint8_t brightness) {
        buffer_[module].brightness = brightness;
        setDirtyBrightness();
    }

    void storeScanLimit(unsigned module, uint8_t scanLimit) {
        buffer_[module].scanLimit = scanLimit;
        setDirtyScanLimit();
    }

    void storeDecodeMode(unsigned module, uint8_t decodeMode) {
        buffer_[module].decodeMode = decodeMode;
        setDirtyDecodeMode();
    }

    void resizeBuffer(unsigned num) {
        buffer_.resize(num);
    }

public:
    // Constructing and destructing is no issue.
    MAX7219() = default;
    ~MAX7219() = default;

    MAX7219(MAX7219 const&) = default;
    MAX7219(MAX7219&&) = default;
    MAX7219& operator=(MAX7219 const&) = default;
    MAX7219& operator=(MAX7219&&) = default;

    /**
     * Gets the write immediately flag.
     * 
     * @return true if write immediately is enabled, false otherwise.
     */
    bool writeImmediately() const { return writeImmediately_; }

    /**
     * Sets the write immediately flag.
     * 
     * @param value The value to set for the write immediately flag.
     */
    void writeImmediately(bool value) { writeImmediately_ = value; }

    void setDirtyBrightness() { dirtyBrightness_ = true; }
    void resetDirtyBrightness() { dirtyBrightness_ = false; }
    bool isDirtyBrightness() const { return dirtyBrightness_; }

    void setDirtyScanLimit() { dirtyScanLimit_ = true; }
    void resetDirtyScanLimit() { dirtyScanLimit_ = false; }
    bool isDirtyScanLimit() const { return dirtyScanLimit_; }

    void setDirtyDecodeMode() { dirtyDecodeMode_ = true; }
    void resetDirtyDecodeMode() { dirtyDecodeMode_ = false; }
    bool isDirtyDecodeMode() const { return dirtyDecodeMode_; }

    void setDirtyBuffer() { dirtyBuffer_ = true; }
    void resetDirtyBuffer() { dirtyBuffer_ = false; }
    bool isDirtyBuffer() const { return dirtyBuffer_; }

    /**
     * Marks all cached data is clean, i.e., the same as what is shown on the display.
     */
    void setClean() {
        resetDirtyBrightness();
        resetDirtyScanLimit();
        resetDirtyDecodeMode();
        resetDirtyBuffer();
    }

    /**
     * Marks all cached data as dirty, i.e., different from what is shown on the display.
     */
    void setDirty() {
        setDirtyBrightness();
        setDirtyScanLimit();
        setDirtyDecodeMode();
        setDirtyBuffer();
    }

    /**
     * Gets the padding flag.
     * 
     * @return true if padding is enabled, false otherwise.
     */
    bool padding() const { return padding_; }

    /**
     * Sets the padding flag.
     * 
     * @param value The value to set for the padding flag.
     */
    void padding(bool value) { padding_ = value; }

    // Access the cached values
    uint8_t getBrightness(uint8_t module) const { return buffer_[module].brightness; }
    uint8_t getScanLimit(uint8_t module) const { return buffer_[module].scanLimit; }
    uint8_t getDecodeMode(uint8_t module) const { return buffer_[module].decodeMode; }
    bool hasValue(uint8_t module) const { return buffer_[module].hasValue; }
    int32_t getValue(uint8_t module) const { return buffer_[module].value; }

    // The actions we can take on this device

    /**
     * Sets the brightness level of all MAX7219 devices.
     * 
     * @param value The brightness level to set (0-15).
     */
    void setBrightness(uint8_t value) {
        for (auto& state: buffer_) {
            state.brightness = value;
        }
        setDirtyBrightness();
        if (writeImmediately()) { sendBrightness(); }
    }

    /**
     * Sets the brightness of a specific MAX7219 device.
     * 
     * @param module The position of the MAX7219 device.
     * @param value The brightness value to set (0-15).
     */
    void setBrightness(uint8_t module, uint8_t value) {
        storeBrightness(module, value);
        setDirtyBrightness();
        if (writeImmediately()) { sendBrightness(); }
    }

    /**
     * Sets the scan limit of all MAX7219 devices.
     * 
     * @param value The scan limit value to set (0-7).
     */
    void setScanLimit(uint8_t value) {
        for (auto& state: buffer_) {
            state.scanLimit = value;
        }
        setDirtyScanLimit();
        if (writeImmediately()) { sendScanLimit(); }
    }

    /**
     * Sets the scan limit of a specific MAX7219 device.
     * 
     * @param module The position of the MAX7219 device.
     * @param value The scan limit value to set (0-7).
     */
    void setScanLimit(uint8_t module, uint8_t value) {
        storeScanLimit(module, value);
        setDirtyScanLimit();
        if (writeImmediately()) { sendScanLimit(); }
    }

    /**
     * Sets the decode mode for all MAX7219 devices.
     *
     * @param value The value to set for the decode mode.
     */
    void setDecodeMode(uint8_t value) {
        for (auto& state: buffer_) {
            state.decodeMode = value;
        }
        setDirtyDecodeMode();
        if (writeImmediately()) { sendDecodeMode(); }
    }

    /**
     * Sets the decode mode of a specific MAX7219 device.
     * 
     * @param module The position of the MAX7219 device.
     * @param value The decode mode value to set (0-255).
     */
    void setDecodeMode(uint8_t module, uint8_t value) {
        storeDecodeMode(module, value);
        setDirtyDecodeMode();
        if (writeImmediately()) { sendDecodeMode(); }
    }

    /**
     * Shuts down all MAX7219 devices.
     * 
     * This function sends the shutdown command to the MAX7219 device, 
     * which turns off all the LEDs and stops the display operation.
     */
    void shutdown() { static_cast<MaxClass*>(this)->doShutdown(); }

    /**
     * Shuts down a specific MAX7219 device.
     * 
     * @param module The position of the MAX7219 device.
     */
    void shutdown(uint8_t module) {static_cast<MaxClass*>(this)->doShutdown(module); }

    /**
     * Performs the startup procedure for all MAX7219 devices.
     * 
     * This function sends the necessary commands to initialize the MAX7219 device.
     * It sets the shutdown mode to normal operation.
     */
    void startup() { static_cast<MaxClass*>(this)->doStartup(); }

    /**
     * Starts up a specific MAX7219 device.
     * 
     * @param module The position of the MAX7219 device.
     */
    void startup(uint8_t module) { static_cast<MaxClass*>(this)->doStartup(module); }

    /**
     * Performs a display test on all MAX7219 devices.
     * 
     * @param value The value to set for the display test. Non-zero value turns on the test, while zero turns it off.
     */
    void displayTest(uint8_t value) { static_cast<MaxClass*>(this)->doDisplayTest(value); }

    /**
     * Performs a display test on a specific MAX7219 device.
     * 
     * @param module The position of the MAX7219 device.
     * @param value The display test value to set (0-1).
     */
    void displayTest(uint8_t module, uint8_t value) { static_cast<MaxClass*>(this)->doDisplayTest(module, value); }

    /**
     * Clears the display of all MAX7219 devices.
     */
    void clear() {
        for (auto& module : buffer()) {
            module.hasValue = false;
            module.value = 0;
            module.buffer.fill(0x0f);
        }
        setDirtyBuffer();
        if (writeImmediately()) sendData();
    }

    /**
     * Clears the display of a specific MAX7219 device.
     * 
     * @param module The module number of the MAX7219 device.
     */
    void clear(uint8_t module) {
        auto& buf = buffer() [module];

        buf.hasValue = false;
        buf.value = 0;
        buf.buffer.fill(0x0f);

        setDirtyBuffer();
        if (writeImmediately()) sendData();
    }

    /**
     * Sets a number to be displayed on a specific MAX7219 device.
     * 
     * @param module The module number of the MAX7219 device.
     * @param value The number to be displayed.
     */
    void setNumber(uint8_t module, int32_t value) {
        auto& buf = buffer() [module];
        buf.hasValue = true;
        buf.value = value;

        bool neg{ false };
        if (value < 0) {
            neg = true;
            value = -value;
        }
        buf.buffer.fill(0x0f);

        for (unsigned pos = 0; (pos < 8); ++pos, value /= 10) {
            if ((value > 0) || padding() || (pos == 0)) {
                buf.buffer[pos] = (value % 10);
            } else if (neg) {
                buf.buffer[pos] = 0x0a;
                neg = false;
            } else {
                buf.buffer[pos] = 0x0f;
            }
        }
        setDirtyBuffer();
        if (writeImmediately()) sendData();
    }

    /**
     * Reset the attached modules.
     */
    void reset() { static_cast<MaxClass*>(this)->doReset(); }
    /**
     * Sends the cached brightness levels.
     */
    void sendBrightness() { static_cast<MaxClass*>(this)->doSendBrightness(); }

    /**
     * Sends the cached scan limits.
     */
    void sendScanLimit() { static_cast<MaxClass*>(this)->doSendScanLimit(); }

    /**
     * Sends the cached decode modes.
     */
    void sendDecodeMode() { static_cast<MaxClass*>(this)->doSendDecodeMode(); }

    /**
     * Sends the cached display data.
     */
    void sendBuffer() { static_cast<MaxClass*>(this)->doSendBuffer(); }


    /**
     * Sends the display data to the MAX7219 device.
     * 
     * This method sends the display data stored in the buffer to the MAX7219 device.
     */
    void sendData() {
        if (isDirtyBrightness()) { sendBrightness(); }
        if (isDirtyScanLimit()) { sendScanLimit(); }
        if (isDirtyDecodeMode()) { sendDecodeMode(); }

        if (isDirtyBuffer()) { sendBuffer(); }
    }

    /**
     * Directly set the buffer's contents.
     */
    void setBuffer(uint8_t module, std::span<uint8_t> data) {
        if (data.size() != MAX7219_DIGITS) {
            return; // Ignore if we're passed the wrong number of bytes.
        }
        auto& buf = buffer()[module];
        for (unsigned i = 0; i < MAX7219_DIGITS; i++) {
            buf.buffer [i] = data [i];
        }
        setDirtyBuffer();
    }
};

} // namespace nl::rakis::raspberrypi::devices