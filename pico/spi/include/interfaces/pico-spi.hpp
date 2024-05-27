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

    enum class DefaultPin : unsigned
    {
        SPI0_MISO = 16,
        SPI0_CS = 17,
        SPI0_SCK = 18,
        SPI0_MOSI = 19
    };

    inline consteval unsigned operator+(DefaultPin pin)
    {
        return unsigned(pin);
    };

    class PicoSPI : public virtual SPI
    {
        bool initialized_{ false };
        spi_inst_t *interface_;

        unsigned csPin_;
        unsigned sckPin_;
        unsigned mosiPin_;
        unsigned misoPin_;

    public:
        PicoSPI(spi_inst_t *interface, unsigned csPin, unsigned sckPin, unsigned mosiPin, unsigned misoPin);

        PicoSPI(unsigned csPin, unsigned sckPin, unsigned mosiPin, unsigned misoPin);

        PicoSPI();

        PicoSPI(const PicoSPI &) = default;
        PicoSPI(PicoSPI &&) = default;
        PicoSPI &operator=(const PicoSPI &) = default;
        PicoSPI &operator=(PicoSPI &&) = default;

        virtual ~PicoSPI() = default;

    protected:
        virtual std::ostream &log() {
            return std::cout;
        }

        inline bool initialized() const { return initialized_; }
        inline void initialized(bool init) { initialized_ = init; }

    public:
        virtual void open() override;

        virtual void close() override;

        virtual void select() override;

        virtual void deselect() override;

        virtual void writeAll(std::array<uint8_t, 2> const &value) override;

        virtual void writeAll(std::function<std::array<uint8_t, 2>(unsigned)> const &value) override;
    };

} // namespace nl::rakis::raspberrypi::interfaces