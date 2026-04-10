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
#error "SPI support is required for the SSD1309 OLED driver"
#endif


#include <devices/monochrome-oled-controller.hpp>

namespace nl::rakis::raspberrypi::devices {


/**
 * The max width supported by the SSD1309 controller is 128 pixels.
 */
static constexpr unsigned SSD1309MaxWidth{ 128 };

/**
 * The max height supported by the SSD1309 controller is 64 lines, in 8 pages of 8 pixels each.
 */
static constexpr unsigned SSD1309MaxHeight{ 64 };


template<class SpiClass>
class SSD1309 : public MonochromeOLEDController<SSD1309<SpiClass>, SpiClass, SSD1309MaxWidth, SSD1309MaxHeight> {

public:
    SSD1309(SpiClass& spi, int reset, int dc)
        : MonochromeOLEDController<SSD1309<SpiClass>, SpiClass, SSD1309MaxWidth, SSD1309MaxHeight>(spi, reset, dc)
    {}
    SSD1309(SpiClass& spi, int reset, int dc, int width, int height)
        : MonochromeOLEDController<SSD1309<SpiClass>, SpiClass, SSD1309MaxWidth, SSD1309MaxHeight>(spi, reset, dc, width, height)
    {}
    ~SSD1309() = default;

    SSD1309(const SSD1309&) = delete;
    SSD1309(SSD1309&&) = default;
    SSD1309& operator=(const SSD1309&) = delete;
    SSD1309& operator=(SSD1309&&) = default;

    void init() {
        this->reset();

        this->log("Send initialization commands to display.");

        this->displayOff();

        this->setContrast(0x8f);

        this->setInverted(false);
        this->setAllOnes(false);

        this->setHorizontalFlip(false);
        this->setVerticalFlip(false);
        this->setStartLine(0);
        this->setRowsScanned(this->height());
        this->setVerticalOffset(0);

        this->setTiming(0x00, 0x08);    //--Set Display Clock Divide Ratio/ Oscillator Frequency

        this->setPreChargePeriod(0x02, 0x02);
        this->setCOMPinConfiguration(true, false);
        this->setDeselectLevel(0x40);

        this->command(0xAD, 0x8B);      // Set DC-DC control (SSD1309-specific; enables internal converter)

        this->displayOn();

        RaspberryPi::instance().sleepMs(100);
    }

};

} // nl::rakis::raspberrypi::devices