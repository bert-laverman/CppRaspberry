#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.


#if !defined(HAVE_SPI)
#error "SPI support is required for the MAX7219 driver"
#endif

#include <cstdint>

#include <array>
#include <vector>

#include <devices/spi_device.hpp>

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
 * @brief Represents a MAX7219 device or a (daisy-chained) set of MAX7219 devices.
 * 
 * This class provides an interface to control a MAX7219 device using SPI communication.
 * It allows setting brightness, scan limit, decode mode, and other parameters of the device.
 * It also provides methods to display numbers and clear the display.
 */
class MAX7219 {
    std::vector<MAX7219Module> buffer_;

    bool dirtyBrightness_{ false };
    bool dirtyScanLimit_{ false };
    bool dirtyDecodeMode_{ false };
    bool dirtyBuffer_{ false };

    bool padding_{ false };
    bool writeImmediately_{ true };

protected:
    inline std::vector<MAX7219Module>& buffer() { return buffer_; }

    /**
     * @brief Sends the cached brightness levels.
     */
    virtual void sendBrightness() = 0;

    /**
     * @brief Sends the cached scan limits.
     */
    virtual void sendScanLimit() = 0;

    /**
     * @brief Sends the cached decode modes.
     */
    virtual void sendDecodeMode() = 0;

    /**
     * @brief Sends the cached display data.
     */
    virtual void sendBuffer() = 0;

    inline void storeBrightness(unsigned module, uint8_t brightness) {
        buffer_[module].brightness = brightness;
        setDirtyBrightness();
    }

    inline void storeScanLimit(unsigned module, uint8_t scanLimit) {
        buffer_[module].scanLimit = scanLimit;
        setDirtyScanLimit();
    }

    inline void storeDecodeMode(unsigned module, uint8_t decodeMode) {
        buffer_[module].decodeMode = decodeMode;
        setDirtyDecodeMode();
    }

public:
    // Constructing and destructing is no issue.
    MAX7219() = default;
    virtual ~MAX7219() = default;

    MAX7219(MAX7219 const&) = default;
    MAX7219(MAX7219&&) = default;
    MAX7219& operator=(MAX7219 const&) = default;
    MAX7219& operator=(MAX7219&&) = default;

    /**
     * @brief Gets the write immediately flag.
     * 
     * @return true if write immediately is enabled, false otherwise.
     */
    inline bool writeImmediately() const { return writeImmediately_; }

    /**
     * @brief Sets the write immediately flag.
     * 
     * @param value The value to set for the write immediately flag.
     */
    inline void writeImmediately(bool value) { writeImmediately_ = value; }

    inline void setDirtyBrightness() { dirtyBrightness_ = true; }
    inline void resetDirtyBrightness() { dirtyBrightness_ = false; }
    inline bool isDirtyBrightness() const { return dirtyBrightness_; }

    inline void setDirtyScanLimit() { dirtyScanLimit_ = true; }
    inline void resetDirtyScanLimit() { dirtyScanLimit_ = false; }
    inline bool isDirtyScanLimit() const { return dirtyScanLimit_; }

    inline void setDirtyDecodeMode() { dirtyDecodeMode_ = true; }
    inline void resetDirtyDecodeMode() { dirtyDecodeMode_ = false; }
    inline bool isDirtyDecodeMode() const { return dirtyDecodeMode_; }

    inline void setDirtyBuffer() { dirtyBuffer_ = true; }
    inline void resetDirtyBuffer() { dirtyBuffer_ = false; }
    inline bool isDirtyBuffer() const { return dirtyBuffer_; }

    /**
     * @brief Marks all cached data is clean, i.e., the same as what is shown on the display.
     */
    inline void setClean() {
        resetDirtyBrightness();
        resetDirtyScanLimit();
        resetDirtyDecodeMode();
        resetDirtyBuffer();
    }

    /**
     * @brief Marks all cached data as dirty, i.e., different from what is shown on the display.
     */
    inline void setDirty() {
        setDirtyBrightness();
        setDirtyScanLimit();
        setDirtyDecodeMode();
        setDirtyBuffer();
    }

    /**
     * @brief Gets the padding flag.
     * 
     * @return true if padding is enabled, false otherwise.
     */
    inline bool padding() const { return padding_; }

    /**
     * @brief Sets the padding flag.
     * 
     * @param value The value to set for the padding flag.
     */
    inline void padding(bool value) { padding_ = value; }

    // Access the cached values
    inline uint8_t getBrightness(uint8_t module) const { return buffer_[module].brightness; }
    inline uint8_t getScanLimit(uint8_t module) const { return buffer_[module].scanLimit; }
    inline uint8_t getDecodeMode(uint8_t module) const { return buffer_[module].decodeMode; }
    inline bool hasValue(uint8_t module) const { return buffer_[module].hasValue; }
    inline int32_t getValue(uint8_t module) const { return buffer_[module].value; }

    // The actions we can take on this device

    /**
     * @brief Sets the brightness level of all MAX7219 devices.
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
     * @brief Sets the brightness of a specific MAX7219 device.
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
     * @brief Sets the scan limit of all MAX7219 devices.
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
     * @brief Sets the scan limit of a specific MAX7219 device.
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
     * @brief Sets the decode mode for all MAX7219 devices.
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
     * @brief Sets the decode mode of a specific MAX7219 device.
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
     * @brief Shuts down all MAX7219 devices.
     * 
     * This function sends the shutdown command to the MAX7219 device, 
     * which turns off all the LEDs and stops the display operation.
     */
    virtual void shutdown() = 0;

    /**
     * @brief Shuts down a specific MAX7219 device.
     * 
     * @param module The position of the MAX7219 device.
     */
    virtual void shutdown(uint8_t module) = 0;

    /**
     * @brief Performs the startup procedure for all MAX7219 devices.
     * 
     * This function sends the necessary commands to initialize the MAX7219 device.
     * It sets the shutdown mode to normal operation.
     */
    virtual void startup() = 0;

    /**
     * @brief Starts up a specific MAX7219 device.
     * 
     * @param module The position of the MAX7219 device.
     */
    virtual void startup(uint8_t module) = 0;

    /**
     * @brief Performs a display test on all MAX7219 devices.
     * 
     * @param value The value to set for the display test. Non-zero value turns on the test, while zero turns it off.
     */
    virtual void displayTest(uint8_t value) = 0;

    /**
     * @brief Performs a display test on a specific MAX7219 device.
     * 
     * @param module The position of the MAX7219 device.
     * @param value The display test value to set (0-1).
     */
    virtual void displayTest(uint8_t module, uint8_t value) = 0;

    /**
     * @brief Clears the display of all MAX7219 devices.
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
     * @brief Clears the display of a specific MAX7219 device.
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
     * @brief Sets a number to be displayed on a specific MAX7219 device.
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
     * @brief Reset the attached modules.
     */
     virtual void reset() = 0;

    /**
     * @brief Sends the display data to the MAX7219 device.
     * 
     * This method sends the display data stored in the buffer to the MAX7219 device.
     */
    void sendData() {
        if (isDirtyBrightness()) { sendBrightness(); }
        if (isDirtyScanLimit()) { sendScanLimit(); }
        if (isDirtyDecodeMode()) { sendDecodeMode(); }

        if (isDirtyBuffer()) { sendBuffer(); }
    }
};

} // namespace nl::rakis::raspberrypi::devices