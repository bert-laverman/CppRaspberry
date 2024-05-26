#pragma once
// Copyright (c) 2024 by Bert Laverman, All rights reserved


#if !defined(HAVE_SPI)
#error "SPI support is required for the MAX7219 driver"
#endif

#include <interfaces/spi.hpp>
#include <devices/max7219.hpp>


namespace nl::rakis::raspberrypi::devices {

/**
 * @brief Represents a MAX7219 device or a (daisy-chained) set of MAX7219 devices.
 * 
 * This class provides an interface to control a MAX7219 device using SPI communication.
 * It allows setting brightness, scan limit, decode mode, and other parameters of the device.
 * It also provides methods to display numbers and clear the display.
 */
class LocalMAX7219 : public MAX7219, public SPIDevice {

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

public:
    /**
     * @brief Constructs a MAX7219 object.
     * 
     * @param spi The SPI interface to communicate with the device.
     */
    LocalMAX7219(interfaces::SPI& spi) : MAX7219(), SPIDevice(spi) {
        interface().device(this);

        init();
    }

    /**
     * @brief Destructor for MAX7219 object.
     */
    virtual ~LocalMAX7219() = default;

protected:
    /**
     * @brief Callback function called when the number of modules changes.
     * 
     * @param num_modules The new number of modules.
     */
    void numModulesChanged(unsigned num_modules) override {
        buffer().resize(num_modules, { .brightness = 7, .scanLimit = 7, .decodeMode = 0, .buffer = { 0 } });
    }

    /**
     * @brief Initializes the MAX7219 device.
     * 
     * This method is called during object construction to initialize the device.
     */
    void init() {
        numModulesChanged(interface().numModules());
    }

public:
    /**
     * @brief Shuts down all MAX7219 devices.
     * 
     * This function sends the shutdown command to the MAX7219 device, 
     * which turns off all the LEDs and stops the display operation.
     */
    void shutdown() override {
        std::array<uint8_t, 2> buf{CMD_SHUTDOWN, 0};
        interface().writeAll(buf);
    }

    /**
     * @brief Shuts down a specific MAX7219 device.
     * 
     * @param pos The position of the MAX7219 device.
     */
    void shutdown(uint8_t pos) override {
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
    void startup() override {
        std::array<uint8_t, 2> buf{CMD_SHUTDOWN, 1};
        interface().writeAll(buf);
    }

    /**
     * @brief Starts up a specific MAX7219 device.
     * 
     * @param pos The position of the MAX7219 device.
     */
    void startup(uint8_t pos) override {
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
    void displayTest(uint8_t value) override {
        std::array<uint8_t, 2> buf{CMD_DISPLAYTEST, value};
        interface().writeAll(buf);
    }

    /**
     * @brief Performs a display test on a specific MAX7219 device.
     * 
     * @param pos The position of the MAX7219 device.
     * @param value The display test value to set (0-1).
     */
    void displayTest(uint8_t pos, uint8_t value) override {
        std::array<uint8_t, 2> buf{CMD_DISPLAYTEST, value};
        interface().writeAll([pos, buf](unsigned module) {
            return (pos == module) ? buf : BUF_NOOP;
        });
    }

    /**
     * @brief Reset the attached modules.
     */
     void reset() override {
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
     * @brief Sends the cached brightness levels to all modules.
     */
    void sendBrightness() override {
        interface().writeAll([this](unsigned module) {
            return std::array<uint8_t, 2>{ CMD_BRIGHTNESS, buffer()[module].brightness };
        });
        resetDirtyBrightness();
    }

    /**
     * @brief Sends the cached scan limits to all modules.
     */
    void sendScanLimit() override {
        interface().writeAll([this](unsigned module) {
            return std::array<uint8_t, 2>{ CMD_SCANLIMIT, buffer()[module].scanLimit };
        });
        resetDirtyScanLimit();
    }

    /**
     * @brief Sends the cached decode modes to all modules.
     */
    void sendDecodeMode() override {
        interface().writeAll([this](unsigned module) {
            return std::array<uint8_t, 2>{ CMD_DECODEMODE, buffer()[module].decodeMode };
        });
        resetDirtyDecodeMode();
    }

    /**
     * @brief Sends the display data to the MAX7219 device.
     * 
     * This method sends the display data stored in the buffer to the MAX7219 device.
     */
    void sendBuffer() override {
        if (isDirtyBuffer()) {
            for (unsigned pos = 0; pos < 8; ++pos) {
                interface().writeAll([pos,this](unsigned module){
                    return std::array<uint8_t, 2>{uint8_t(CMD_DIGIT0 + pos), buffer()[module].buffer[pos]};
                });
            }
            resetDirtyBuffer();
        }
    }
};

} // namespace nl::rakis::raspberrypi::devices