#pragma once
/*
 * Copyright (c) 2023-2024 by Bert Laverman. All Rights Reserved.
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

#include <array>
#include <functional>
#include <string>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include <interfaces/spi.hpp>

namespace nl::rakis::raspberrypi::interfaces
{

    constexpr static const char *spi0{"/dev/spidev0.0"};
    constexpr static const char *spi1{"/dev/spidev0.1"};

    inline char hex(int value)
    {
        return char(value < 10 ? '0' + value : 'a' + value - 10);
    }

    class Zero2WSPI : public virtual SPI
    {
        std::string interface_;
        int fd_{ -1 };
        int channel_{ -1 };
        unsigned int baudRate_{ 10*1000*1000 };

    protected:
        virtual std::ostream &log() override {
            return std::cerr;
        }

        [[maybe_unused]]
        void writeBlocking_spidev(std::function<std::array<uint8_t, 2>(unsigned)> const &value);

        [[maybe_unused]]
        void writeBlocking_pigpiod(std::function<std::array<uint8_t, 2>(unsigned)> const &value);

    public:
        Zero2WSPI(const char *interface)
            : SPI(), interface_(interface)
        {
        }

        Zero2WSPI() : Zero2WSPI(spi0)
        {
        }

        Zero2WSPI(Zero2WSPI const &) = default;
        Zero2WSPI(Zero2WSPI &&) = default;
        Zero2WSPI &operator=(Zero2WSPI const &) = default;
        Zero2WSPI &operator=(Zero2WSPI &&) = default;

        virtual ~Zero2WSPI()
        {
            close();
        }

        inline unsigned int baudRate() const { return baudRate_; }
        inline void baudRate(unsigned int baudRate) { baudRate_ = baudRate; }

        virtual void open() override;

        virtual void close() override;

        virtual void select() override;

        virtual void deselect() override;

        virtual void writeAll(std::array<uint8_t, 2> const &value) override;

        virtual void writeAll(std::function<std::array<uint8_t, 2>(unsigned)> const &value) override;

    };

} // namespace nl::rakis::raspberrypi::interfaces::zero2w