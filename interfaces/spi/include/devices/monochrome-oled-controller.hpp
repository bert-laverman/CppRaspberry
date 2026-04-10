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


#include <algorithm>
#include <vector>
 
#include <devices/spi-7pin-display.hpp>


namespace nl::rakis::raspberrypi::devices {


/**
 * MonoChromeOLEDController supports the most typical OLED controllers.
 * 
 * @param SpiDevice The actual controller class. (CRTP)
 * @param SpiClass The actial SPI interface class.
 * @param MaxWidth The max supported display width, typically 128 or 132.
 * @param MaxHeight The max supported display height, typically 32, 64, or 128.
 */
template <class SpiDevice, class SpiClass,
          unsigned MaxWidth, unsigned MaxHeight>
class MonochromeOLEDController : public SPI7PinDisplay<SpiDevice, SpiClass, MaxWidth, MaxHeight>
{
    enum class AddressingMode : uint8_t {
        Horizontal  = 0x00,
        Vertical    = 0x01,
        Page        = 0x02
    };

    AddressingMode addressingMode_{ AddressingMode::Page };

    bool sendImediately_{ false };

    uint8_t columnOffset_{ 0 };
    bool pamOnly_{ false };

public:
    MonochromeOLEDController(SpiClass& spi, int reset, int dc)
        : SPI7PinDisplay<SpiDevice, SpiClass, MaxWidth, MaxHeight>(spi, reset, dc)
    {}
    MonochromeOLEDController(SpiClass& spi, int reset, int dc, unsigned width, unsigned height)
        : SPI7PinDisplay<SpiDevice, SpiClass, MaxWidth, MaxHeight>(spi, reset, dc, width, height)
    {}
    ~MonochromeOLEDController() = default;

    MonochromeOLEDController(const MonochromeOLEDController&) = delete;
    MonochromeOLEDController(MonochromeOLEDController&&) = default;
    MonochromeOLEDController& operator=(const MonochromeOLEDController&) = delete;
    MonochromeOLEDController& operator=(MonochromeOLEDController&&) = default;


    /**
     * If set to true, any changes to the buffer are immediately sent to the display.
     */
    void sendImmediately(bool doIt =true) noexcept { sendImediately_ = doIt; }

    /**
     * If set to true, any changes to the buffer are immediately sent to the display.
     */
    bool isSendImmediately() const noexcept { return sendImediately_; }

    /**
     * The number of columns the controller needs to skip before the actual display starts.
     */
    void columnOffset(uint8_t offset) noexcept { columnOffset_ = offset; }

    /**
     * The number of columns the controller needs to skip before the actual display starts.
     */
    uint8_t columnOffset() const noexcept { return columnOffset_; }

    void pamOnly(bool on) noexcept { pamOnly_ = on; }
    bool pamOnly() const noexcept { return pamOnly_; }

    // Display control

    void setAllOnes(bool on =true) { this->command(on ? 0xa5 : 0xa4); }
    void setInverted(bool on =true) { this->command(on ? 0xa7 : 0xa6); }
    void setHorizontalFlip(bool on =true) { this->command(on ? 0xa1 : 0xa0); }
    void setVerticalFlip(bool on =true) { this->command(on ? 0xc8 : 0xc0); }
    void setVerticalOffset(uint8_t offset) { this->command(0xd3, offset); }
    void setStartLine(uint8_t first =0) { this->command(0x40 | first); }
    void setRowsScanned(uint8_t ratio) { this->command(0xa8, ratio-1); } // Default 63. Parameter to command is one less than value to be used.
    void displayOff() { this->command(0xae); }
    void displayOn() { this->command(0xaf); }

    void setContrast(uint8_t contrast) { this->command(0x81, contrast); }

    /**
     * @param divRatio Display clock divide ratio. (becomes low nibble)
     * @param freq Oscillator frequency. (becomes high niubble)
     */
    void setTiming(uint8_t divRatio, uint8_t freq) { this->command(0xd5, (freq << 4) | (divRatio & 0x0f)); }

    void setCOMPinConfiguration(bool altConfig =true, bool enableLRRemap =false) { this->command(0xda, 0x02 | (altConfig ? 0x10 : 0x00) | (enableLRRemap ? 0x20 : 0x00)); }

    void setPreChargePeriod(uint8_t phase1, uint8_t phase2) { this->command(0xd9, (phase2 << 4) | (phase1 & 0x0f)); }

    void setDeselectLevel(uint8_t level) { this->command(0xdb, level); }

    void setChargePump(bool on) { this->command(0x8d, on ? 0x14 : 0x10); } // SSD1306/SH1106

    // Addressing modes

    /**
     * Set Addressing mode when transferring display data.
     * 
     * Each line is broken into blocks of 8 pixels and a "line" of such blocks is called a page. Subsequent bytes of data are
     * accepted as subsequent columns of 8 pixels. The addressing mode determines the ordering of these blocks as they are transferred.
     * 
     * Supported by 1106, 1305, 1306, 1309
     */
    void setAddressingMode(AddressingMode mode) {
        if (!pamOnly()) {
            addressingMode_ = mode; this->command(0x20, static_cast<uint8_t>(mode));
        } else {
            this->log("PAM only controller: Not switching mode.");
        }
    }

    /**
     * Set Addressing mode to Horizontal Addressing Mode when transferring display data. In this mode, data is stored from left to right.
     * When the end of the page has been reached, transfer will continue at the first column of the next page.
     * 
     * Supported by 1106, 1305, 1306, 1309
     */
    void setHorizontalAddressingMode() { setAddressingMode(AddressingMode::Horizontal); }

    /**
     * Set Addressing mode to Vertical Addressing Mode when transferring display data. In this mode, data is stored from top to bottom.
     * When the last page has been reached, transfer will continue at the first page of the next column.
     * 
     * Supported by 1106, 1305, 1306, 1309
     */
    void setVerticalAddressingMode() { setAddressingMode(AddressingMode::Vertical); }

    /**
     * Set the first and last page to store data to in Horizontal or Vertical Addressing Modes.
     * 
     * @param first The first page. Default is page 0.
     * @param last The last page. Default is the last controller supported page.
     */
    void setPageRange(uint8_t first =0, uint8_t last =((MaxHeight/8)-1)) { this->command(0x22, first, last); }

    /**
     * Set the first and last column to store data to in Horizontal or Vertical Addressing Modes.
     * 
     * @param first The first colum. Default is column 0.
     * @param last The last column. Default is the last controller supported column.
     */
    void setColumnRange(uint8_t first =0, uint8_t last =(MaxWidth-1)) { this->command(0x21, first, last); }

    /**
     * Set Addressing mode to Page Addressing Mode (PAM) when transferring display data. In this mode, data is stored from left to right,
     * but not continue on the next page when the last column has been reached.
     * 
     * Supported by 1106, 1305, 1306, 1309
     */
    void setPageAddressingMode() { setAddressingMode(AddressingMode::Page); }

    /**
     * Set the page number to transfer next.
     * 
     * @param page The page number to send next, ranging from 0 to 7.
     */
    void setPAMPage(uint8_t page) { this->command(0xb0 | page); }

    /**
     * Set the first column to store data to in Page Addressing Mode. Because storage will not continue to the next page, setting
     * the last column is noot needed.
     * 
     * @param first The first column. Default is column 0.
     */
    void setPAMFirstColumn(uint8_t first =0) { this->command(0x00 | (first & 0b00001111), 0x10 | (first >> 4)); }

    //

    
    void sendBuffer() {
        if (this->isDirty()) {
            switch (addressingMode_) {
            case AddressingMode::Page:
            {
                this->log("Sending pages: ", false);
                unsigned pos{ 0 };
                for (uint8_t page = 0; page < (this->height() / 8); page++, pos += this->width()) {
                    this->log(std::format("{} (pos={})... ", page, pos, false), false);

                    this->setPAMPage(page);
                    this->setPAMFirstColumn(columnOffset()); 

                    this->data(std::span<uint8_t>{ &(this->buffer()[pos]), this->width() });
                }
                this->log(std::format("Done for {} by {}", this->width(), this->height()));
            }
                break;

            case AddressingMode::Horizontal:
                this->log(std::format("Setting dimensions to pages {}-{}, and columns {}-{}.", 0, ((MaxHeight/8)-1), columnOffset(), this->columnOffset()+this->width()-1));

                this->setColumnRange(columnOffset(), this->columnOffset()+this->width()-1);
                this->setPageRange(0, ((MaxHeight/8)-1));

                this->log(std::format("Sending {} bytes.", this->buffer().size()));
                this->data(this->buffer());
                break;

            case AddressingMode::Vertical:
                this->log("Vertical addressingmode not implemented.");
                break;
            }
            this->clean();
        }
    }

    void clear() {
        std::fill(this->buffer().begin(), this->buffer().end(), 0);
        this->dirty();
        if (this->isSendImmediately()) { this->sendBuffer(); }
    }

    void set(unsigned x, unsigned y, unsigned color) {
        if ((x < this->width()) && (y < this->height())) {
            if (color) {
                this->buffer()[x + ((y >> 3) * this->width())] |= 0b0000'0001 << (y & 0x07);
            }
            else {
                this->buffer()[x + ((y >> 3) * this->width())] &= ~(0b0000'0001 << (y & 0x07));
            }
        }
        this->dirty();
        if (this->isSendImmediately()) { this->sendBuffer(); }
    }

    void set(unsigned x, unsigned y) {
        if ((x < this->width()) && (y < this->height())) {
            this->buffer()[x + ((y >> 3) * this->width())] |= 0b0000'0001 << (y & 0x07);
        }
        this->dirty();
        if (this->isSendImmediately()) { this->sendBuffer(); }
    }

    void clear(unsigned x, unsigned y) {
        if ((x < this->width()) && (y < this->height())) {
            this->buffer()[x + ((y >> 3) * this->width())] &= ~(0b0000'0001 << (y & 0x07));
        }
        this->dirty();
        if (this->isSendImmediately()) { this->sendBuffer(); }
    }

};

} // nl::rakis::raspberrypi::devices
