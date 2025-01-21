#pragma once
/*
 * Copyright (c) 2024-2025 by Bert Laverman. All Rights Reserved.
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


#include <span>
#include <memory>

#include <util/verbose-component.hpp>
#include <interfaces/spi.hpp>


namespace nl::rakis::raspberrypi::devices {


/**
 * @brief This represents a device connected to a SPI interface, possibly daisy-chained.
 */
template <class SpiClass>
class SPIDevice : public util::VerboseComponent
{
    SpiClass& spi_;

protected:
    virtual void numDevicesChanged() = 0;

public:
    SPIDevice(SpiClass& spi) : spi_(spi) {}
    ~SPIDevice() {}

    SPIDevice(const SPIDevice&) = default;
    SPIDevice(SPIDevice&&) = default;
    SPIDevice& operator=(const SPIDevice&) = default;
    SPIDevice& operator=(SPIDevice&&) = default;

    SpiClass& interface() const { return spi_; }

};

} // namespace nl::rakis::raspberrypi::devices