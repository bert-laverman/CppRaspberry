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


#include <fcntl.h>

#include <vector>
#include <thread>
#include <iostream>

#include <interfaces/i2c.hpp>


namespace nl::rakis::raspberrypi::interfaces {


constexpr static char const *i2cdev_bus1 = "/dev/i2c-1";
constexpr static char const *i2cdev_bus2 = "/dev/i2c-2";

class I2CDevI2C : public I2C {
    std::string interface_{ i2cdev_bus1 };
    int fd_{ -1 };

public:
    I2CDevI2C() = default;
    I2CDevI2C(I2CDevI2C const &) = delete;
    I2CDevI2C(I2CDevI2C &&) = default;
    I2CDevI2C &operator=(I2CDevI2C const &) = delete;
    I2CDevI2C &operator=(I2CDevI2C &&) = default;

    virtual ~I2CDevI2C();

    I2CDevI2C(std::string interface) : interface_(interface) {}

    inline std::string const &interface() const noexcept { return interface_; }
    inline void interface(std::string const &interface) { interface_ = interface; }

    virtual void open() override;

    virtual void close() override;

    virtual bool canListen() const noexcept override;

    virtual void startListening() override;

    virtual void stopListening() override;

    virtual bool canSend() const noexcept override;

    bool write(uint8_t address, std::span<uint8_t> data) override;

};

} // namespace nl::rakis::raspberrypi::interfaces