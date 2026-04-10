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
#error "SPI support is required for the MCP23S17 IO Expander"
#endif

#include <span>
#include <array>

#include <interfaces/spi.hpp>
#include <devices/spi-device.hpp>

namespace nl::rakis::raspberrypi::devices {


template <class SpiClass>
class MCP23S17 : public SPIDevice<SpiClass>
{
public:
    uint8_t readRegister(uint8_t addr, uint8_t reg) {
        std::array<uint8_t, 2> readCmd{{ /*0x41*/addr, reg }};
        uint8_t data[8];

        this->interface().select();
        // this->interface().write(readCmd);
        spi_write_blocking(spi0, readCmd.data(), readCmd.size());
        spi_read_blocking(spi0, 0, std::addressof(data[0]), sizeof(data));
        // spi_write_read_blocking(spi0, readCmd.data(), &data[0], readCmd.size());
        this->interface().deselect();

        return data[0];
    }

    void writeRegister(uint8_t reg, uint8_t value) {
        std::array<uint8_t, 3> writeCmd{{ 0x40, reg, value }};

        this->interface().select();
        this->interface().write(writeCmd);
        this->interface().deselect();
    }

public:
    MCP23S17(SpiClass& spi) : SPIDevice<SpiClass>(spi) {}
    ~MCP23S17() = default;

    MCP23S17(const MCP23S17&) = default;
    MCP23S17& operator=(const MCP23S17&) = default;

    MCP23S17(MCP23S17&&) = delete;
    MCP23S17& operator=(MCP23S17&&) = delete;

    void init() {
        std::array<uint8_t, 1> resetCmd{{ 0 }};

        this->interface().select();
        this->interface().write(resetCmd);
        RaspberryPi::instance().sleepMs(100);
        this->interface().deselect();
    }

};

} // nl::rakis::raspberrypi::devices