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
#error "SPI support is required for the SH1305 OLED driver"
#endif


#include <devices/monochrome-oled-controller.hpp>

namespace nl::rakis::raspberrypi::devices {


/**
 * The max width supported by the SH1106 controller is 132 pixels.
 */
static constexpr unsigned SH1106MaxWidth{ 132 };

/**
 * The max height supported by the SH1106 controller is 64 lines, in 8 pages of 8 pixels each.
 */
static constexpr unsigned SH1106MaxHeight{ 64 };


template<class SpiClass>
class SH1106 : public MonochromeOLEDController<SH1106<SpiClass>, SpiClass, SH1106MaxWidth, SH1106MaxHeight> {

public:
    SH1106(SpiClass& spi, int reset, int dc)
        : MonochromeOLEDController<SH1106<SpiClass>, SpiClass, SH1106MaxWidth, SH1106MaxHeight>(spi, reset, dc)
    {}
    SH1106(SpiClass& spi, int reset, int dc, unsigned width, unsigned height)
        : MonochromeOLEDController<SH1106<SpiClass>, SpiClass, SH1106MaxWidth, SH1106MaxHeight>(spi, reset, dc, width, height)
    {}
    ~SH1106() = default;

    SH1106(const SH1106&) = delete;
    SH1106(SH1106&&) = default;
    SH1106& operator=(const SH1106&) = delete;
    SH1106& operator=(SH1106&&) = default;

    void init() {
        this->pamOnly(true);

        this->reset();

        this->log("Send initialization commands to display.");

        this->displayOff();                     // Display OFF

        this->setPAMFirstColumn(0);
        this->setContrast(0x7F);                // Brightness?

        this->setInverted(false);               // Normal display (not inverted)
        this->setAllOnes(false);                // Display follows RAM content

        this->setHorizontalFlip(false);
        this->setVerticalFlip(false);
        this->setStartLine(0);
        this->setRowsScanned(this->height());   // Default
        this->setVerticalOffset(0);

        this->setTiming(0x00, 0x08);            // Divider ratio = 0, Frequency = 8);

        this->setPreChargePeriod(0x01, 0x0f);
        this->setCOMPinConfiguration(true, false);
        this->setDeselectLevel(0x40);

        this->setChargePump(true);

        this->displayOn();                      // Display ON

        RaspberryPi::instance().sleepMs(100);
    }

};

} // nl::rakis::raspberrypi::devices