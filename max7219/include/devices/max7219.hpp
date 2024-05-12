#pragma once
// Copyright 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
// Purpose: MAX7219 LED driver

#if !defined(HAVE_SPI)
#error "SPI support is required for the MAX7219 driver"
#endif

#include <array>
#include <vector>
#include <span>

#include <interfaces/spi.hpp>
#include <devices/spi_device.hpp>


namespace nl::rakis::raspberrypi::devices {

constexpr static const unsigned MAX7219_DIGITS = 8;

struct MAX7219Module {
    uint8_t brightness{ 0 };
    uint8_t scanLimit{ 7 };
    uint8_t decodeMode{ 0 };
    std::array<uint8_t, MAX7219_DIGITS> buffer;
};

/**
 * @brief Represents a MAX7219 device or a (daisy-chained) set of MAX7219 devices.
 * 
 * This class provides an interface to control a MAX7219 device using SPI communication.
 * It allows setting brightness, scan limit, decode mode, and other parameters of the device.
 * It also provides methods to display numbers and clear the display.
 */
class MAX7219 : public SPIDevice {

    // Constants for MAX7219 commands
    constexpr static const uint8_t CMD_NOOP = 0x00;
    constexpr static const uint8_t CMD_DIGIT0 = 0x01;
    constexpr static const uint8_t CMD_DIGIT1 = 0x02;
    constexpr static const uint8_t CMD_DIGIT2 = 0x03;
    constexpr static const uint8_t CMD_DIGIT3 = 0x04;
    constexpr static const uint8_t CMD_DIGIT4 = 0x05;
    constexpr static const uint8_t CMD_DIGIT5 = 0x06;
    constexpr static const uint8_t CMD_DIGIT6 = 0x07;
    constexpr static const uint8_t CMD_DIGIT7 = 0x08;
    constexpr static const uint8_t CMD_DECODEMODE = 0x09;
    constexpr static const uint8_t CMD_BRIGHTNESS = 0x0A;
    constexpr static const uint8_t CMD_SCANLIMIT = 0x0B;
    constexpr static const uint8_t CMD_SHUTDOWN = 0x0C;
    constexpr static const uint8_t CMD_DISPLAYTEST = 0x0F;

    // Buffer for NOOP command
    constexpr static std::array<uint8_t, 2> BUF_NOOP{CMD_NOOP, 0};

    std::vector<MAX7219Module> buffer_; // Buffer for storing display data
    bool dirtyBrightness{ false };
    bool dirtyScanLimit{ false };
    bool dirtyDecodeMode{ false };
    bool dirtyBuffer{ false };

    bool padding_{ false }; // Flag to enable/disable padding
    bool writeImmediately_{ true }; // Flag to control immediate write to the device

    inline void setDirtyBrightness() { dirtyBrightness = true; }
    inline void resetDirtyBrightness() { dirtyBrightness = false; }
    inline bool isDirtyBrightness() const { return dirtyBrightness; }
    inline void storeBrightness(unsigned module, uint8_t brightness) { buffer_[module].brightness = brightness; setDirtyBrightness(); }

    inline void setDirtyScanLimit() { dirtyScanLimit = true; }
    inline void resetDirtyScanLimit() { dirtyScanLimit = false; }
    inline bool isDirtyScanLimit() const { return dirtyScanLimit; }
    inline void storeScanLimit(unsigned module, uint8_t scanLimit) { buffer_[module].scanLimit = scanLimit; setDirtyScanLimit(); }

    inline void setDirtyDecodeMode() { dirtyDecodeMode = true; }
    inline void resetDirtyDecodeMode() { dirtyDecodeMode = false; }
    inline bool isDirtyDecodeMode() const { return dirtyDecodeMode; }
    inline void storeDecodeMode(unsigned module, uint8_t decodeMode) { buffer_[module].decodeMode = decodeMode; setDirtyDecodeMode(); }

    inline void setDirtyBuffer() { dirtyBuffer = true; }
    inline void resetDirtyBuffer() { dirtyBuffer = false; }
    inline bool isDirtyBuffer() const { return dirtyBuffer; }

public:
    /**
     * @brief Constructs a MAX7219 object.
     * 
     * @param spi The SPI interface to communicate with the device.
     */
    MAX7219(interfaces::SPI& spi) : SPIDevice(spi) {
        interface().device(this);

        init();
    }

    /**
     * @brief Destructor for MAX7219 object.
     */
    virtual ~MAX7219() = default;

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

    /**
     * @brief Callback function called when the number of modules changes.
     * 
     * @param num_modules The new number of modules.
     */
    virtual void numModulesChanged(unsigned num_modules) override {
        buffer_.resize(num_modules, { .brightness = 7, .scanLimit = 7, .decodeMode = 0, .buffer = { 0 } });
    }

    /**
     * @brief Initializes the MAX7219 device.
     * 
     * This method is called during object construction to initialize the device.
     */
    void init() {
        numModulesChanged(interface().numModules());
    }

    /**
     * @brief Sends the cached brightness levels to all modules.
     */
    void sendBrightness() {
        interface().writeAll([this](unsigned module) {
            return std::array<uint8_t, 2>{ CMD_BRIGHTNESS, buffer_[module].brightness };
        });
        resetDirtyBrightness();
    }

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
     * @param pos The position of the MAX7219 device.
     * @param value The brightness value to set (0-15).
     */
    void setBrightness(uint8_t pos, uint8_t value) {
        storeBrightness(pos, value);
        setDirtyBrightness();
        if (writeImmediately()) { sendBrightness(); }
    }

    /**
     * @brief Sends the cached scan limits to all modules.
     */
    void sendScanLimit() {
        interface().writeAll([this](unsigned module) {
            return std::array<uint8_t, 2>{ CMD_SCANLIMIT, buffer_[module].scanLimit };
        });
        resetDirtyScanLimit();
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
     * @param pos The position of the MAX7219 device.
     * @param value The scan limit value to set (0-7).
     */
    void setScanLimit(uint8_t pos, uint8_t value) {
        storeScanLimit(pos, value);
        setDirtyScanLimit();
        if (writeImmediately()) { sendScanLimit(); }
    }

    /**
     * @brief Sends the cached decode modes to all modules.
     */
    void sendDecodeMode() {
        interface().writeAll([this](unsigned module) {
            return std::array<uint8_t, 2>{ CMD_DECODEMODE, buffer_[module].decodeMode };
        });
        resetDirtyDecodeMode();
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
     * @param pos The position of the MAX7219 device.
     * @param value The decode mode value to set (0-255).
     */
    void setDecodeMode(uint8_t pos, uint8_t value) {
        storeDecodeMode(pos, value);
        setDirtyDecodeMode();
        if (writeImmediately()) { sendDecodeMode(); }
    }

    /**
     * @brief Shuts down all MAX7219 devices.
     * 
     * This function sends the shutdown command to the MAX7219 device, 
     * which turns off all the LEDs and stops the display operation.
     */
    void shutdown() {
        std::array<uint8_t, 2> buf{CMD_SHUTDOWN, 0};
        interface().writeAll(buf);
    }

    /**
     * @brief Shuts down a specific MAX7219 device.
     * 
     * @param pos The position of the MAX7219 device.
     */
    void shutdown(uint8_t pos) {
        std::array<uint8_t, 2> buf{CMD_SHUTDOWN, 0};
        interface().writeAll([pos, buf](unsigned module) {
            return (pos == module) ? buf : BUF_NOOP;
        });
    }

    /**
     * @brief Performs the startup procedure for all MAX7219 devices.
     * 
     * This function sends the necessary commands to initialize the MAX7219 device.
     * It sets the shutdown mode to normal operation.
     */
    void startup() {
        std::array<uint8_t, 2> buf{CMD_SHUTDOWN, 1};
        interface().writeAll(buf);
    }

    /**
     * @brief Starts up a specific MAX7219 device.
     * 
     * @param pos The position of the MAX7219 device.
     */
    void startup(uint8_t pos) {
        std::array<uint8_t, 2> buf{CMD_SHUTDOWN, 1};
        interface().writeAll([pos, buf](unsigned module) {
            return (pos == module) ? buf : BUF_NOOP;
        });
    }

    /**
     * @brief Performs a display test on all MAX7219 devices.
     * 
     * @param value The value to set for the display test. Non-zero value turns on the test, while zero turns it off.
     */
    void displayTest(uint8_t value) {
        std::array<uint8_t, 2> buf{CMD_DISPLAYTEST, value};
        interface().writeAll(buf);
    }

    /**
     * @brief Performs a display test on a specific MAX7219 device.
     * 
     * @param pos The position of the MAX7219 device.
     * @param value The display test value to set (0-1).
     */
    void displayTest(uint8_t pos, uint8_t value) {
        std::array<uint8_t, 2> buf{CMD_DISPLAYTEST, value};
        interface().writeAll([pos, buf](unsigned module) {
            return (pos == module) ? buf : BUF_NOOP;
        });
    }

    /**
     * @brief Clears the display of all MAX7219 devices.
    */
    inline void clear() {
        for (auto& module : buffer_) {
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
    inline void clear(unsigned module) {
        buffer_[module].buffer.fill(0x0f);
        setDirtyBuffer();
        if (writeImmediately()) sendData();
    }

    /**
     * @brief Sets a number to be displayed on a specific MAX7219 device.
     * 
     * @param module The module number of the MAX7219 device.
     * @param value The number to be displayed.
     */
    inline void setNumber(unsigned module, uint32_t value) {
        buffer_[module].buffer.fill(0);

        for (unsigned pos = 0; (pos < 8); ++pos, value /= 10) {
            if ((value > 0) || padding_ || (pos == 0)) {
                buffer_[module].buffer[pos] = (value % 10);
            } else {
                buffer_[module].buffer[pos] = 0x0f;
            }
        }
        setDirtyBuffer();
        if (writeImmediately()) sendData();
    }

    /**
     * @brief Reset the attached modules.
     */
     inline void reset() {
        writeImmediately(true);
        shutdown();
        displayTest(0);
        setScanLimit(7);
        setDecodeMode(255);
        startup();
        setBrightness(7);
        clear();
        writeImmediately(false);
     }

    /**
     * @brief Sends the display data to the MAX7219 device.
     * 
     * This method sends the display data stored in the buffer to the MAX7219 device.
     */
    inline void sendData() {
        if (isDirtyBrightness()) { sendBrightness(); }
        if (isDirtyScanLimit()) { sendScanLimit(); }
        if (isDirtyDecodeMode()) { sendDecodeMode(); }

        if (isDirtyBuffer()) {
            for (unsigned pos = 0; pos < 8; ++pos) {
                interface().writeAll([pos,this](unsigned module){
                    return std::array<uint8_t, 2>{uint8_t(CMD_DIGIT0 + pos), buffer_[module].buffer[pos]};
                });
            }
            resetDirtyBuffer();
        }
    }
};

} // namespace nl::rakis::raspberrypi::devices