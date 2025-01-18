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

#include <vector>

#include <pico/stdlib.h>
#include "hardware/i2c.h"

#include <interfaces/i2c.hpp>


namespace nl::rakis::raspberrypi::interfaces {

class PicoI2C : public I2C
{
    constexpr static const uint baudrate = 100000;

    i2c_inst_t *interface_{ nullptr };

    inline uint8_t readByteRaw() {
        return i2c_read_byte_raw(interface_);
    }
    inline void readBytesRaw(std::vector<uint8_t> &date) {
        i2c_read_raw_blocking(interface_, date.data(), date.size());
    }

public:
    PicoI2C(i2c_inst_t *interface, unsigned sdaPin, unsigned sclPin);

    PicoI2C(unsigned sdaPin, unsigned sclPin) : PicoI2C(i2c0, sdaPin, sclPin)
    {
    }

    PicoI2C() : PicoI2C(i2c0, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN)
    {
    }

    PicoI2C(const PicoI2C &) = default;
    PicoI2C(PicoI2C &&) = default;
    PicoI2C &operator=(const PicoI2C &) = default;
    PicoI2C &operator=(PicoI2C &&) = default;

    ~PicoI2C() = default;

    inline int channel() const { return i2c_hw_index(interface_); }

    static PicoI2C& defaultInstance();

    virtual void open() override;

    virtual void close() override;

    virtual bool canListen() const noexcept override;

    virtual void startListening() override;

    virtual void stopListening() override;

    virtual bool canSend() const noexcept override;

    bool write(uint8_t address, std::span<uint8_t> data) override;

};

} // namespace nl::rakis::raspberrypi::interfaces