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

class Zero2WI2C : public I2C {
    int channel_{ -1 };
    std::jthread listener_;

    std::vector<uint8_t> bytes_;

    void processBytes(std::span<uint8_t> data);

    void listen();

protected:
    bool readOneByte(uint8_t &value);

    inline void channel(int channel) { channel_ = channel; }

    void startListening();
    void stopListening();

public:
    Zero2WI2C() = default;
    Zero2WI2C(Zero2WI2C const &) = delete;
    Zero2WI2C(Zero2WI2C &&) = default;
    Zero2WI2C &operator=(Zero2WI2C const &) = delete;
    Zero2WI2C &operator=(Zero2WI2C &&) = default;

    virtual ~Zero2WI2C() {}

    std::ostream &log() override {
        return std::cerr;
    }

    inline int channel() const { return channel_; }

    bool readMessage(protocols::MsgHeader &header, std::vector<uint8_t> &data);
};

constexpr static char const *i2cdev_bus1 = "/dev/i2c-1";
constexpr static char const *i2cdev_bus2 = "/dev/i2c-2";

class Zero2WI2C_i2cdev : public Zero2WI2C {
    std::string interface_{ i2cdev_bus1 };
    int fd_{ -1 };

protected:
    inline int fd() const { return fd_; }

public:
    Zero2WI2C_i2cdev() = default;
    Zero2WI2C_i2cdev(Zero2WI2C_i2cdev const &) = delete;
    Zero2WI2C_i2cdev(Zero2WI2C_i2cdev &&) = default;
    Zero2WI2C_i2cdev &operator=(Zero2WI2C_i2cdev const &) = delete;
    Zero2WI2C_i2cdev &operator=(Zero2WI2C_i2cdev &&) = default;

    virtual ~Zero2WI2C_i2cdev();

    Zero2WI2C_i2cdev(std::string interface) : interface_(interface) {}

    inline std::string const &interface() const { return interface_; }
    inline void interface(std::string const &interface) { interface_ = interface; }

    void open() override;
    void close() override;

    void switchToControllerMode() override;
    void switchToResponderMode(uint8_t address, protocols::MsgCallback cb) override;

    bool readBytes(uint8_t address, std::span<uint8_t> data) override;
    bool writeBytes(uint8_t address, std::span<uint8_t> data) override;

};

class Zero2WI2C_pigpio : public Zero2WI2C {
    int channel_{ -1 };
    std::jthread listener_;
    bool listening_{ false };

    std::vector<uint8_t> bytes_;

    void processBytes(std::span<uint8_t> data);

    static void listen(Zero2WI2C_pigpio& bus);

protected:
    bool readOneByte(uint8_t &value);

    inline void channel(int channel) { channel_ = channel; }
    inline void listening(bool listening) { listening_ = listening; }

    void startListening();
    void stopListening();

public:
    Zero2WI2C_pigpio() = default;
    Zero2WI2C_pigpio(Zero2WI2C_pigpio const &) = delete;
    Zero2WI2C_pigpio(Zero2WI2C_pigpio &&) = default;
    Zero2WI2C_pigpio &operator=(Zero2WI2C_pigpio const &) = delete;
    Zero2WI2C_pigpio &operator=(Zero2WI2C_pigpio &&) = default;

    virtual ~Zero2WI2C_pigpio();

    inline int channel() const { return channel_; }
    inline bool listening() const { return listening_; }

    void open() override;
    void close() override;

    void switchToControllerMode() override;
    void switchToResponderMode(uint8_t address, protocols::MsgCallback cb) override;

    bool readBytes(uint8_t address, std::span<uint8_t> data) override;
    bool writeBytes(uint8_t address, std::span<uint8_t> data) override;

    bool readMessage(protocols::MsgHeader &header, std::vector<uint8_t> &data);
};

} // namespace nl::rakis::raspberrypi::interfaces::zero2w