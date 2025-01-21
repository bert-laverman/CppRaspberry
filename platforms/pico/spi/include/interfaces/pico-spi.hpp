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


#include <cstdint>

#include <array>
#include <functional>

#include <pico/stdlib.h>
#include <hardware/spi.h>

#include <interfaces/spi.hpp>

namespace nl::rakis::raspberrypi::interfaces
{


class PicoSPI : public SPI
{
    bool initialized_{ false };
    int busNr_;
    spi_inst_t *interface_;

public:
    PicoSPI(int busNr, unsigned csPin, unsigned sclkPin, unsigned mosiPin, unsigned misoPin);

    PicoSPI();

    PicoSPI(const PicoSPI &) = default;
    PicoSPI(PicoSPI &&) = default;
    PicoSPI &operator=(const PicoSPI &) = default;
    PicoSPI &operator=(PicoSPI &&) = default;

    virtual ~PicoSPI() = default;

    bool initialized() const { return initialized_; }
    void initialized(bool init) { initialized_ = init; }

    virtual operator bool() const noexcept override { return initialized(); }

    virtual void open() override;

    virtual void close() override;

    virtual void select() override;

    virtual void deselect() override;

    virtual void write(const std::span<uint8_t> data) override;

};

} // namespace nl::rakis::raspberrypi::interfaces