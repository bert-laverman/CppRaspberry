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

#include <interfaces/spi.hpp>
#include <devices/spi-device.hpp>
#include <devices/chainable-spi-device.hpp>

#include <devices/max7219.hpp>


namespace nl::rakis::raspberrypi::devices {

/**
 * Represents a MAX7219 device or a (daisy-chained) set of MAX7219 devices.
 * 
 * This class provides an interface to control a MAX7219 device using SPI communication.
 * It allows setting brightness, scan limit, decode mode, and other parameters of the device.
 * It also provides methods to display numbers and clear the display.
 */
template <class SpiClass>
class LocalMAX7219
    : public MAX7219<LocalMAX7219<SpiClass>>
    , public SPIDevice<SpiClass>
    , public ChainableSPIDevice<LocalMAX7219<SpiClass>>
{

    // Constants for MAX7219 commands
    constexpr static const uint8_t CMD_NOOP   = 0x00;
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
     * Constructs a MAX7219 object.
     * 
     * @param spi The SPI interface to communicate with the device.
     */
    LocalMAX7219(SpiClass& spi) : SPIDevice<SpiClass>(spi) {
        this->resizeBuffer(this->numDevices());   // Make sure everyone agrees
    }

    LocalMAX7219(const LocalMAX7219&) = default;
    LocalMAX7219(LocalMAX7219&&) = default;
    LocalMAX7219& operator=(const LocalMAX7219&) = default;
    LocalMAX7219& operator=(LocalMAX7219&&) = default;

    ~LocalMAX7219() {}

    void numDevicesChanged() {
        this->resizeBuffer(this->numDevices());
    }


protected:

    /**
     * Send a single command to all modules.
     */
    void sendAll(uint8_t cmd, uint8_t par =0) {
        const unsigned size{ this->numDevices() * 2 };
        std::vector<uint8_t> buf(size, par);

        for (unsigned i{0}; i < size; i += 2) {
            buf [i] = cmd;
        }
        this->interface().write(buf);
    }

    /**
     * Send a command to one module, CMD_NOOP to all the others.
     */
    void sendOne(unsigned mod, uint8_t cmd, uint8_t par =0) {
        const unsigned size{ this->numDevices() * 2 };
        std::vector<uint8_t> buf(size, par);

        for (unsigned i{0}; i < size; i += 2) {
            buf [i] = (i == mod*2) ? cmd : CMD_NOOP;
        }
        this->interface().write(buf);
    }

    void sendEach(std::function<void(unsigned mod, uint8_t& cmd, uint8_t& par)> getData) {
        const unsigned size{ this->numDevices() * 2 };
        std::vector<uint8_t> buf(size, 0);

        for (unsigned i{0},mod{0}; i < size; i += 2, mod++) {
            getData(mod, buf[i], buf[i+1]);
        }
        this->interface().write(buf);
    }

public:

    /**
     * Shuts down all MAX7219 devices.
     * 
     * This function sends the shutdown command to the MAX7219 device, 
     * which turns off all the LEDs and stops the display operation.
     */
    void doShutdown() {
        sendAll(CMD_SHUTDOWN);
    }

    /**
     * Shuts down a specific MAX7219 device.
     * 
     * @param pos The position of the MAX7219 device.
     */
    void doShutdown(uint8_t pos) {
        sendOne(pos, CMD_SHUTDOWN);
    }

    /**
     * Performs the startup procedure for all MAX7219 devices.
     * 
     * This function sends the necessary commands to initialize the MAX7219 device.
     * It sets the shutdown mode to normal operation.
     */
    void doStartup() {
        sendAll(CMD_SHUTDOWN, 1);
    }

    /**
     * Starts up a specific MAX7219 device.
     * 
     * @param pos The position of the MAX7219 device.
     */
    void doStartup(uint8_t pos) {
        sendOne(pos, CMD_SHUTDOWN, 1);
    }

    /**
     * Performs a display test on all MAX7219 devices.
     * 
     * @param value The value to set for the display test. Non-zero value turns on the test, while zero turns it off.
     */
    void doDisplayTest(uint8_t value) {
        sendAll(CMD_DISPLAYTEST, value);
    }

    /**
     * Performs a display test on a specific MAX7219 device.
     * 
     * @param pos The position of the MAX7219 device.
     * @param value The display test value to set (0-1).
     */
    void doDisplayTest(uint8_t pos, uint8_t value) {
        sendOne(pos, CMD_DISPLAYTEST, value);
    }

    /**
     * Reset the attached modules.
     */
     void doReset() {
        this->writeImmediately(true);
        this->shutdown();
        this->displayTest(0);
        this->setScanLimit(7);
        this->setDecodeMode(255);
        this->startup();
        this->setBrightness(7);
        this->clear();
        this->writeImmediately(false);
     }

    /**
     * Sends the cached brightness levels to all modules.
     */
    void doSendBrightness() {
        sendEach([this](unsigned mod, uint8_t& cmd, uint8_t& par) {
            cmd = CMD_BRIGHTNESS;
            par = this->buffer()[mod].brightness;
        });
        this->resetDirtyBrightness();
    }

    /**
     * Sends the cached scan limits to all modules.
     */
    void doSendScanLimit() {
        sendEach([this](unsigned mod, uint8_t& cmd, uint8_t& par) {
            cmd = CMD_SCANLIMIT;
            par = this->buffer()[mod].scanLimit;
        });
        this->resetDirtyScanLimit();
    }

    /**
     * Sends the cached decode modes to all modules.
     */
    void doSendDecodeMode() {
        sendEach([this](unsigned mod, uint8_t& cmd, uint8_t& par) {
            cmd = CMD_DECODEMODE;
            par = this->buffer()[mod].decodeMode;
        });
        this->resetDirtyDecodeMode();
    }

    /**
     * Sends the display data to the MAX7219 device.
     * 
     * This method sends the display data stored in the buffer to the MAX7219 device.
     */
    void doSendBuffer() {
        if (this->isDirtyBuffer()) {
            for (unsigned pos = 0; pos < 8; ++pos) {
                sendEach([this, pos](unsigned mod, uint8_t& cmd, uint8_t& par) {
                    cmd = CMD_DIGIT0 + pos;
                    par = this->buffer()[mod].buffer[pos];
                });
            }
            this->resetDirtyBuffer();
        }
    }
};

} // namespace nl::rakis::raspberrypi::devices