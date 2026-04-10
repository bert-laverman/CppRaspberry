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
#error "SPI support is required for the SSD1305 OLED driver"
#endif


#include <devices/monochrome-oled-controller.hpp>

namespace nl::rakis::raspberrypi::devices {


/**
 * The max width supported by the SSD1305 controller is 132 pixels.
 */
static constexpr unsigned SSD1305MaxWidth{ 132 };

/**
 * The max height supported by the SSD1305 controller is 64 lines, in 8 pages of 8 pixels each.
 */
static constexpr unsigned SSD1305MaxHeight{ 64 };


template<class SpiClass>
class SSD1305 : public MonochromeOLEDController<SSD1305<SpiClass>, SpiClass, SSD1305MaxWidth, SSD1305MaxHeight> {

public:
    SSD1305(SpiClass& spi, int reset, int dc)
        : MonochromeOLEDController<SSD1305<SpiClass>, SpiClass, SSD1305MaxWidth, SSD1305MaxHeight>(spi, reset, dc)
    {}
    SSD1305(SpiClass& spi, int reset, int dc, unsigned width, unsigned height)
        : MonochromeOLEDController<SSD1305<SpiClass>, SpiClass, SSD1305MaxWidth, SSD1305MaxHeight>(spi, reset, dc, width, height)
    {}
    ~SSD1305() = default;

    SSD1305(const SSD1305&) = delete;
    SSD1305(SSD1305&&) = default;
    SSD1305& operator=(const SSD1305&) = delete;
    SSD1305& operator=(SSD1305&&) = default;

    void init() {
        this->reset();

        this->log("Send initialization commands to display.");

        this->displayOff();

        this->setPAMFirstColumn(0);
        this->setContrast(0x80);

        this->setInverted(false);
        this->setAllOnes(false);

        this->setHorizontalFlip(false);
        this->setVerticalFlip(false);
        this->setStartLine(0);
        this->setRowsScanned(this->height());
        this->setVerticalOffset(0);

        this->setTiming(0x00, 0x0f);

        this->setPreChargePeriod(0x0c, 0x02);
        this->setCOMPinConfiguration(true, false);
        this->setDeselectLevel(0x08);

        this->command(0xD8, 0x05);   // NEW: Set area color mode & low power mode

        this->displayOn();

        RaspberryPi::instance().sleepMs(100);

        // <!-- Original demo code -->
        // this->command(0x04);//--Set Lower Column Start Address for Page Addressing Mode	
        // this->command(0x10);//--Set Higher Column Start Address for Page Addressing Mode
        // this->command(0x40);//---Set Display Start Line
        // this->command(0x81);//---Set Contrast Control for BANK0
        // this->command(0x80);//--Contrast control register is set
        // this->command(0xA1);//--Set Segment Re-map
        // this->command(0xA6);//--Set Normal/Inverse Display
        // this->command(0xA8);//--Set Multiplex Ratio
        // this->command(0x1F);
        // this->command(0xC8);//--Set COM Output Scan Direction
        // this->command(0xD3);//--Set Display Offset
        // this->command(0x00);
        // this->command(0xD5);//--Set Display Clock Divide Ratio/ Oscillator Frequency
        // this->command(0xF0);
        // this->command(0xD8);//--Set Area Color Mode ON/OFF & Low Power Display Mode
        // this->command(0x05);
        // this->command(0xD9);//--Set pre-charge period
        // this->command(0xC2);
        // this->command(0xDA);//--Set COM Pins Hardware Configuration
        // this->command(0x12);
        // this->command(0xDB);//--Set VCOMH Deselect Level
        // this->command(0x08);//--Set VCOM Deselect Level
        // <!-- End original demo code -->
    }

};

} // nl::rakis::raspberrypi::devices