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
#error "SPI support is required for the SH1122 OLED driver"
#endif


#include <raspberry-pi.hpp>
#include <devices/grayscale-oled-controller.hpp>


namespace nl::rakis::raspberrypi::devices {


/**
 * Maximum width supported by the SH1122 controller: 256 pixels.
 */
static constexpr unsigned SH1122MaxWidth{ 256 };

/**
 * Maximum height supported by the SH1122 controller: 64 rows.
 */
static constexpr unsigned SH1122MaxHeight{ 64 };


/**
 * Driver for the SH1122 256×64 16-level grayscale OLED controller.
 *
 * The SH1122 is connected via a 7-pin SPI interface (CS, SCLK, MOSI, DC, RST).
 * It stores pixel data as 4 bits per pixel (16 grayscale levels), with two
 * pixels packed per byte.  The display RAM is accessed using page addressing:
 * 8 pages of 8 rows each, 128 column-addresses per page (each addressing two
 * adjacent pixels).
 *
 * Typical usage:
 * @code
 *   SH1122<PicoSPI> display{ spi, RESET_PIN, DC_PIN };
 *   display.init();
 *   display.set(10, 20, 8);   // mid-grey pixel at (10, 20)
 *   display.sendBuffer();
 * @endcode
 *
 * @param SpiClass The concrete SPI interface class to use.
 */
template <class SpiClass>
class SH1122 : public GrayscaleOLEDController<SH1122<SpiClass>, SpiClass, SH1122MaxWidth, SH1122MaxHeight>
{
public:
    SH1122(SpiClass& spi, int reset, int dc)
        : GrayscaleOLEDController<SH1122<SpiClass>, SpiClass, SH1122MaxWidth, SH1122MaxHeight>(spi, reset, dc)
    {}
    SH1122(SpiClass& spi, int reset, int dc, unsigned width, unsigned height)
        : GrayscaleOLEDController<SH1122<SpiClass>, SpiClass, SH1122MaxWidth, SH1122MaxHeight>(spi, reset, dc, width, height)
    {}
    ~SH1122() = default;

    SH1122(const SH1122&) = delete;
    SH1122(SH1122&&) = default;
    SH1122& operator=(const SH1122&) = delete;
    SH1122& operator=(SH1122&&) = default;


    /**
     * Set the current page address (row group of 8 display rows).
     * Used by the base class `sendBuffer()` via CRTP.
     *
     * @param page Page index, 0–7.
     */
    void setPageAddress(uint8_t page) { this->command(0xB0 | (page & 0x0F)); }

    /**
     * Set the current column address (each unit = 2 display pixels).
     * Sends the low nibble followed by the high nibble command.
     * Used by the base class `sendBuffer()` via CRTP.
     *
     * @param col Column address, 0–127 for a 256-pixel-wide display.
     */
    void setColumnAddress(uint8_t col) {
        setLowColumnAddress(col);
        setHighColumnAddress(col);
    }

    /**
     * Force all pixels to maximum brightness, ignoring the frame buffer.
     *
     * @param on Pass true to force all pixels on, false to resume normal display.
     */
    void setAllOnes(bool on = true) { this->command(on ? 0xA5 : 0xA4); }

    /**
     * Turn the display panel on (normal operation).
     */
    void powerOn() { this->command(0xAF); }

    /**
     * Turn the display panel off (sleep mode).
     */
    void powerOff() { this->command(0xAE); }

    /**
     * Set the oscillator frequency.
     *
     * @param freq Oscillator frequency value.
     */
    void setOscillatorFreq(uint8_t freq) { this->command(0xD5, freq); }
    
    /**
     * Set the number of active display rows (multiplex ratio).
     *
     * @param ratio Number of rows to drive, 1–64.  The command value sent is ratio-1.
     */
    void setMultiplexRatio(uint8_t ratio) { this->command(0xA8, static_cast<uint8_t>(ratio - 1)); }

    /**
     * Shift the display image vertically in RAM by the given number of rows.
     *
     * @param offset Vertical offset in rows, 0–63.
     */
    void setDisplayOffsetMod(uint8_t offset) { this->command(0xD3, offset); }

    /**
     * Set the column address for subsequent data writes.
     *
     * @param addr Column address, 0–127 (each address corresponds to two pixels).
     */
    void setRowAddr(uint8_t addr) { this->command(0xB0 | (addr & 0x0F)); }

    /**
     * Set the high nibble of the column address for subsequent data writes.
     * 
     * @param addr Column address, 0–127 (each address corresponds to two pixels).
     */
    void setHighColumnAddress(uint8_t addr) { this->command(0x10 | (addr >> 4)); }

    /**
     * Set the low nibble of the column address for subsequent data writes.
     * 
     * @param addr Column address, 0–127 (each address corresponds to two pixels).
     */
    void setLowColumnAddress(uint8_t addr) { this->command(0x00 | (addr & 0x0F)); }

    /**
     * Set the display RAM row that maps to the top of the visible area.
     *
     * @param line Display start row, 0–63.
     */
    void setStartLine(uint8_t line = 0) { this->command(0x40 | (line & 0x3F)); }

    /**
     * Set the discharge level for the VCOM output during the pre-charge period.
     * 
     * @param level Discharge level, 0–15 (controller-specific voltage range).
     */
    void setDischargeLevel(uint8_t level) { this->command(0x30 | (level & 0x0F)); }

    /**
     * Set onboard oled DC-DC voltage converter status and switch frequency command.
     * 
     * @param mode DC-DC control mode, 0–255 (controller-specific range).
     */
    void setDcDcControlMod(uint8_t mode) { this->command(0xAD, mode); }

    /**
     * Mirror the display horizontally (segment re-map).
     *
     * @param on Pass true to reverse column order, false for normal order.
     */
    void setHorizontalFlip(bool on = true) { this->command(on ? 0xA1 : 0xA0); }

    /**
     * Mirror the display vertically (COM output scan direction).
     *
     * @param on Pass true to scan from COM[N-1] to COM0, false for normal order.
     */
    void setVerticalFlip(bool on = true) { this->command(on ? 0xC8 : 0xC0); }

    /**
     * Set the display contrast (brightness).
     *
     * @param contrast Contrast value, 0 (minimum) to 255 (maximum).
     */
    void setContrast(uint8_t contrast) { this->command(0x81, contrast); }

    /**
     * Set the pre-charge period for both phases.
     *
     * @param phase1 Phase 1 period in DCLK units (1–15).
     * @param phase2 Phase 2 period in DCLK units (1–15).
     */
    void setPreChargePeriod(uint8_t phase1, uint8_t phase2) { this->command(0xD9, static_cast<uint8_t>((phase2 << 4) | (phase1 & 0x0F))); }

    /**
     * Set common pad output voltage at deselect command.
     * 
     * @param level Voltage level, 0–63 (controller-specific voltage range).
     */
    void setVCOM(uint8_t level) { this->command(0xDB, level); }

    /**
     * Set the VSEGM output voltage level for the segment driver.
     *
     * @param level Voltage level byte (controller-specific range).
     */
    void setVSEG(uint8_t level) { this->command(0xDC, level); }

    /**
     * Invert the displayed image (swap pixel on/off state).
     *
     * @param on Pass true to invert, false for normal polarity.
     */
    void setInvertedIntensity(bool on = true) { this->command(on ? 0xA7 : 0xA6); }


    /**
     * Initialise the SH1122 controller with a recommended default configuration
     * suitable for a 256×64 display panel.
     *
     * Performs a hardware reset, configures the multiplex ratio, scan direction,
     * clock, charge pump and contrast, clears the frame buffer, and turns the
     * display on.  A 100 ms delay is inserted after power-on to allow the panel
     * voltage to stabilise.
     */
    void init() {
        this->reset();

        this->log("Initialising SH1122 display.");

        this->powerOff();
        // send all initialization commands
        this->setOscillatorFreq(0x50U);
        this->setMultiplexRatio(this->height());
        this->setDisplayOffsetMod(0x00U);

        this->setRowAddr(0x00U);
        this->setHighColumnAddress(0x00U);
        this->setLowColumnAddress(0x00U);
        this->setStartLine(0x00U);

        this->setDischargeLevel(0x00U);
        this->setDcDcControlMod(0x80U);

        this->setHorizontalFlip(false);
        this->setVerticalFlip(false);

        this->setContrast(0x90);
        this->setPreChargePeriod(/* 0x28U*/ 8, 2);
        this->setVCOM(0x30U);
        this->setVSEG(0x1EU);

        this->setInvertedIntensity(false);

        this->clear();

        this->powerOn(); // power back on oled

        RaspberryPi::instance().sleepMs(100);
    }

};

} // namespace nl::rakis::raspberrypi::devices
