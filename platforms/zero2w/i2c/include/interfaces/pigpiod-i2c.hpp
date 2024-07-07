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


#include <fcntl.h>

#include <vector>
#include <thread>
#include <iostream>

#include <interfaces/i2c.hpp>


namespace nl::rakis::raspberrypi::interfaces {



/**
 * @brief The basic pigpiod-based implementation can only act as controller. (Formerly known as "Master")
 */
class PigpiodI2C : public I2C {
    int channel_{ -1 };
    int bus_{ 1 };

protected:

    void channel(int c) noexcept { channel_ = c; }

    int channel() const noexcept { return channel_; }

    void bus(int b) noexcept { bus_ = b; }

    int bus() const noexcept { return bus_; }

    void error(int result, int bus =0, uint8_t address =0, unsigned size =0);

public:
    PigpiodI2C() = default;

    PigpiodI2C(PigpiodI2C const &) = delete;
    PigpiodI2C(PigpiodI2C &&) = default;
    PigpiodI2C &operator=(PigpiodI2C const &) = delete;
    PigpiodI2C &operator=(PigpiodI2C &&) = default;

    virtual ~PigpiodI2C();

    virtual void open() override;

    virtual void close() override;

    virtual bool canListen() const noexcept override;

    virtual void startListening() override;

    virtual void stopListening() override;

    virtual bool canSend() const noexcept override;

    bool write(uint8_t address, std::span<uint8_t> data) override;

};


/**
 * @brief The BSC version can only act as receiver. (formerly known as "Slave")
 */
class PigpiodBSCI2C : public I2C {
    int channel_{ -1 };
    std::jthread listener_;
    bool listening_{ false };

    std::vector<uint8_t> bytes_;

    void processBytes(std::span<uint8_t> data);

    static void listen(PigpiodBSCI2C& bus);

protected:

    inline void channel(int channel) { channel_ = channel; }

    inline int channel() const { return channel_; }

public:
    PigpiodBSCI2C() = default;

    PigpiodBSCI2C(PigpiodBSCI2C const &) = delete;
    PigpiodBSCI2C(PigpiodBSCI2C &&) = default;
    PigpiodBSCI2C &operator=(PigpiodBSCI2C const &) = delete;
    PigpiodBSCI2C &operator=(PigpiodBSCI2C &&) = default;

    virtual ~PigpiodBSCI2C();

    virtual void open() override;

    virtual void close() override;

    virtual bool canListen() const noexcept override;

    virtual void startListening() override;

    virtual void stopListening() override;

    virtual bool canSend() const noexcept override;

    bool write(uint8_t address, std::span<uint8_t> data) override;

};


} // namespace nl::rakis::raspberrypi::interfaces::zero2w