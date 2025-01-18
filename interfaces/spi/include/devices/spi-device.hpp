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


#include <span>
#include <memory>


#include <util/verbose-component.hpp>


namespace nl::rakis::raspberrypi::interfaces {
    class SPI;
} // namespace nl::rakis::raspberrypi::interfaces


namespace nl::rakis::raspberrypi::devices {


/**
 * @brief This represents a device connected to a SPI interface, possibly daisy-chained.
 */
class SPIDevice : public util::VerboseComponent
{
    std::shared_ptr<interfaces::SPI> spi_;
    unsigned numDevices_{ 1 };

protected:
    virtual void numDevicesChanged() = 0;

public:
    SPIDevice() {}

    SPIDevice(const SPIDevice&) = delete;
    SPIDevice(SPIDevice&&) = default;
    SPIDevice& operator=(const SPIDevice&) = delete;
    SPIDevice& operator=(SPIDevice&&) = default;

    virtual ~SPIDevice() = default;

    std::shared_ptr<interfaces::SPI> interface() const { return spi_; }
    void interface(std::shared_ptr<interfaces::SPI> spi) { spi_ = spi; }

    unsigned numDevices() const noexcept { return numDevices_; }
    void numDevices(unsigned num) { numDevices_ = num; numDevicesChanged(); }

};

} // namespace nl::rakis::raspberrypi::devices