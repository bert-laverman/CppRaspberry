#pragma once
/*
 * Copyright (c) 2025 by Bert Laverman. All Rights Reserved.
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
#error "SPI support is required for the ST7789 TFT driver"
#endif


#include <raspberry-pi.hpp>
#include <devices/color-tft-controller.hpp>


namespace nl::rakis::raspberrypi::devices {


/**
 * Maximum width supported by the ST7789 controller: 320 pixels.
 */
static constexpr unsigned ST7789MaxWidth{ 320 };

/**
 * Maximum height supported by the ST7789 controller: 240 pixels.
 */
static constexpr unsigned ST7789MaxHeight{ 240 };


/**
 * Driver for the ST7789 RGB565 color TFT controller.
 *
 * The ST7789 is connected via a 7-pin SPI interface (CS, SCLK, MOSI, DC, RST).
 * It uses window-based addressing: CASET sets the column range, RASET sets the
 * row range, and RAMWR begins the pixel data stream.
 *
 * Many panels using the ST7789 controller have a physical size smaller than the
 * controller's 320×240 RAM.  Pass the actual panel dimensions to the constructor
 * and supply the correct RAM offsets to `init()` for your specific panel.
 *
 * Typical usage:
 * @code
 *   ST7789<PicoSPI> display{ spi, RST_PIN, DC_PIN, 284, 76 };
 *   display.init(18, 82);  // column and row offsets for this panel
 *   display.set(10, 20, 0xF800);  // red pixel
 *   display.sendBuffer();
 * @endcode
 *
 * @param SpiClass The concrete SPI interface class to use.
 */
template <class SpiClass>
class ST7789 : public ColorTFTController<ST7789<SpiClass>, SpiClass, ST7789MaxWidth, ST7789MaxHeight>
{
    unsigned colOffset_{ 0 };
    unsigned rowOffset_{ 0 };

public:
    ST7789(SpiClass& spi, int reset, int dc, int bl = interfaces::NO_PIN)
        : ColorTFTController<ST7789<SpiClass>, SpiClass, ST7789MaxWidth, ST7789MaxHeight>(spi, reset, dc, bl)
    {}
    ST7789(SpiClass& spi, int reset, int dc, unsigned width, unsigned height, int bl = interfaces::NO_PIN)
        : ColorTFTController<ST7789<SpiClass>, SpiClass, ST7789MaxWidth, ST7789MaxHeight>(spi, reset, dc, width, height, bl)
    {}
    ~ST7789() = default;

    ST7789(const ST7789&) = delete;
    ST7789(ST7789&&) = default;
    ST7789& operator=(const ST7789&) = delete;
    ST7789& operator=(ST7789&&) = default;

    unsigned colOffset() const noexcept { return colOffset_; }
    unsigned rowOffset() const noexcept { return rowOffset_; }

    /**
     * Reconfigure the display without a hardware reset.
     * Useful for trying different orientations, inversion, and RAM offsets at runtime.
     *
     * @param colOffset Column offset into controller RAM.
     * @param rowOffset Row offset into controller RAM.
     * @param madctl    Memory data access control byte.
     * @param invert    Enable display inversion.
     */
    void configure(unsigned colOffset, unsigned rowOffset, uint8_t madctl, bool invert) {
        colOffset_ = colOffset;
        rowOffset_ = rowOffset;
        setMemoryAccessControl(madctl);
        if (invert) invertOn(); else invertOff();
    }


    // -------------------------------------------------------------------------
    // Basic display control
    // -------------------------------------------------------------------------

    /** Turn the display on. */
    void displayOn()  { this->command(0x29); }

    /** Turn the display off (panel goes dark but controller keeps RAM). */
    void displayOff() { this->command(0x28); }

    /** Enter sleep mode (lowest power; display off). */
    void sleepIn()    { this->command(0x10); }

    /** Exit sleep mode. The display needs ~120 ms to recover. */
    void sleepOut()   { this->command(0x11); }

    /** Invert display colours. */
    void invertOn()   { this->command(0x21); }

    /** Restore normal (non-inverted) display. */
    void invertOff()  { this->command(0x20); }


    // -------------------------------------------------------------------------
    // Configuration commands
    // -------------------------------------------------------------------------

    /**
     * Set the pixel format.  Call once during init before sending any pixels.
     * The parameter byte is sent in data mode (DC=HIGH) as required by the ST7789.
     *
     * @param fmt 0x55 for RGB565 (default), 0x66 for RGB666.
     */
    void setPixelFormat(uint8_t fmt = 0x55) {
        this->command(0x3A);
        std::array<uint8_t, 1> p{{ fmt }};
        this->data(std::span<uint8_t>{ p.data(), p.size() });
    }

    /**
     * Set the memory data access control register (scan order, colour order).
     * The parameter byte is sent in data mode (DC=HIGH) as required by the ST7789.
     *
     * @param madctl MADCTL byte.  Common values:
     *   0x00 — normal portrait, top-left origin.
     *   0x60 — landscape (90°), top-left origin.
     *   0xC0 — inverted portrait (180°).
     *   0xA0 — inverted landscape (270°).
     */
    void setMemoryAccessControl(uint8_t madctl) {
        this->command(0x36);
        std::array<uint8_t, 1> p{{ madctl }};
        this->data(std::span<uint8_t>{ p.data(), p.size() });
    }

    /**
     * Set the column address window (CASET).
     * The command byte is sent in command mode (DC=LOW); the four address bytes
     * are sent in data mode (DC=HIGH) as required by the ST7789.
     *
     * @param x0 First column (inclusive), in controller RAM coordinates.
     * @param x1 Last column (inclusive), in controller RAM coordinates.
     */
    void setColumnAddress(uint16_t x0, uint16_t x1) {
        this->command(0x2A);
        std::array<uint8_t, 4> p{{
            static_cast<uint8_t>(x0 >> 8), static_cast<uint8_t>(x0 & 0xFF),
            static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1 & 0xFF) }};
        this->data(std::span<uint8_t>{ p.data(), p.size() });
    }

    /**
     * Set the row address window (RASET).
     * The command byte is sent in command mode (DC=LOW); the four address bytes
     * are sent in data mode (DC=HIGH) as required by the ST7789.
     *
     * @param y0 First row (inclusive), in controller RAM coordinates.
     * @param y1 Last row (inclusive), in controller RAM coordinates.
     */
    void setRowAddress(uint16_t y0, uint16_t y1) {
        this->command(0x2B);
        std::array<uint8_t, 4> p{{
            static_cast<uint8_t>(y0 >> 8), static_cast<uint8_t>(y0 & 0xFF),
            static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1 & 0xFF) }};
        this->data(std::span<uint8_t>{ p.data(), p.size() });
    }

    /**
     * Begin a memory write sequence (RAMWR).
     * Pixel data bytes must follow immediately via `data()`.
     */
    void beginWrite() { this->command(0x2C); }

    /**
     * Set the write window and issue RAMWR, ready for pixel data.
     * Called by the base class `sendBuffer()` via CRTP.
     *
     * @param x0 Left edge of the window (display coordinates).
     * @param y0 Top edge of the window (display coordinates).
     * @param x1 Right edge of the window (display coordinates).
     * @param y1 Bottom edge of the window (display coordinates).
     */
    void prepareWrite(unsigned x0, unsigned y0, unsigned x1, unsigned y1) {
        setColumnAddress(
            static_cast<uint16_t>(x0 + colOffset_),
            static_cast<uint16_t>(x1 + colOffset_));
        setRowAddress(
            static_cast<uint16_t>(y0 + rowOffset_),
            static_cast<uint16_t>(y1 + rowOffset_));
        beginWrite();
    }


    // -------------------------------------------------------------------------
    // Initialisation
    // -------------------------------------------------------------------------

    /**
     * Initialise the ST7789 controller with a standard RGB565 configuration.
     *
     * Performs a hardware reset, brings the panel out of sleep, configures the
     * pixel format and memory access, then turns the display and backlight on.
     * A delay between `sleepOut()` and `displayOn()` allows the panel voltage to settle.
     *
     * Many ST7789 panels (particularly IPS panels) require display inversion to show
     * correct colours; `invert` defaults to false. Enable it if colours appear inverted.
     *
     * @param colOffset Column offset into the controller's RAM for this panel
     *                  (0 for a full 320-wide panel; 18 for a common 284-wide panel).
     * @param rowOffset Row offset into the controller's RAM for this panel
     *                  (0 for a full 240-tall panel; 82 for a common 76-tall panel).
     * @param madctl    Memory data access control byte (default 0x00 = normal portrait).
     * @param invert    Enable display inversion (default true; required by most IPS panels).
     */
    void init(unsigned colOffset = 0, unsigned rowOffset = 0, uint8_t madctl = 0x00, bool invert = false) {
        colOffset_ = colOffset;
        rowOffset_ = rowOffset;

        this->reset();

        this->log("Initialising ST7789 display.");

        // Software reset — controller needs 150 ms to recover.
        this->command(0x01);
        RaspberryPi::instance().sleepMs(150);

        sleepOut();
        RaspberryPi::instance().sleepMs(120);

        setPixelFormat(0x55);           // RGB565
        setMemoryAccessControl(madctl);
        if (invert) invertOn(); else invertOff();
        RaspberryPi::instance().sleepMs(10);

        // Normal display mode on (exit partial mode if previously set).
        this->command(0x13);
        RaspberryPi::instance().sleepMs(10);

        this->clear();
        this->sendBuffer();             // blank the panel before turning on

        displayOn();
        RaspberryPi::instance().sleepMs(20);

        this->backlightOn();
    }

    // Bring sendBuffer into scope (hidden by the using-declaration below)
    using ColorTFTController<ST7789<SpiClass>, SpiClass, ST7789MaxWidth, ST7789MaxHeight>::sendBuffer;

};

} // namespace nl::rakis::raspberrypi::devices
