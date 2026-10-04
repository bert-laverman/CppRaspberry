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

#include <devices/max7219.hpp>

#include <protocols/messages.hpp>
#include <protocols/max7219-messages.hpp>


namespace nl::rakis::raspberrypi::devices {

/**
 * @brief A MAX7219 that is attached to another device, which we reach over a protocol driver (I2C, for instance).
 *
 * The device runs a protocols::MAX7219Handler on a LocalMAX7219, and does what our messages tell it to. We keep the state
 * of the display here as well (the MAX7219 base class does that), and send what is new: brightness, scan limit and decode
 * mode per module, and the contents of a module as a number, or as "blank".
 *
 * Only numbers (setNumber()) and blanking (clear()) can be sent; contents that were set with setBuffer() are not sent.
 *
 * @tparam DriverType The protocol driver, which provides sendMessage(Command, uint8_t address, Msg&).
 */
template <typename DriverType>
class RemoteMAX7219 : public MAX7219<RemoteMAX7219<DriverType>> {
    using Base = MAX7219<RemoteMAX7219<DriverType>>;

    DriverType& driver_;
    uint8_t address_;
    unsigned numDevices_{ 1 };

    bool send(uint8_t module, protocols::Max7219Command command, int32_t value = 0) {
        protocols::MsgMax7219 msg{};
        msg.module = module;
        msg.command = protocols::toValue(command);
        msg.value = value;
        return driver_.sendMessage(protocols::Command::Max7219, address_, msg);
    }

    bool sendToAll(protocols::Max7219Command command, int32_t value = 0) {
        return send(protocols::MsgMax7219::AllModules, command, value);
    }

public:
    RemoteMAX7219(DriverType& driver, uint8_t address) : driver_(driver), address_(address) {
        this->resizeBuffer(numDevices_);
    }

    RemoteMAX7219(const RemoteMAX7219&) = delete;
    RemoteMAX7219(RemoteMAX7219&&) = delete;
    RemoteMAX7219& operator=(const RemoteMAX7219&) = delete;
    RemoteMAX7219& operator=(RemoteMAX7219&&) = delete;

    ~RemoteMAX7219() = default;

    /** The address of the device that the MAX7219 is attached to. */
    uint8_t address() const { return address_; }
    void address(uint8_t address) { address_ = address; }

    /** The number of (chained) MAX7219 modules. This must be the same as on the device. */
    unsigned numDevices() const { return numDevices_; }
    void numDevices(unsigned num) {
        numDevices_ = num;
        this->resizeBuffer(num);
    }

    // The MAX7219 base class calls these.

    void doShutdown() { sendToAll(protocols::Max7219Command::Shutdown); }
    void doShutdown(uint8_t module) { send(module, protocols::Max7219Command::Shutdown); }

    void doStartup() { sendToAll(protocols::Max7219Command::Startup); }
    void doStartup(uint8_t module) { send(module, protocols::Max7219Command::Startup); }

    void doDisplayTest(uint8_t value) { sendToAll(protocols::Max7219Command::TestDisplay, value); }
    void doDisplayTest(uint8_t module, uint8_t value) { send(module, protocols::Max7219Command::TestDisplay, value); }

    /**
     * Reset the device, and our idea of its state: after a reset it shows nothing, at brightness 7, with all digits
     * decoded and all digits scanned.
     */
    void doReset() {
        sendToAll(protocols::Max7219Command::Reset);
        for (auto& module : this->buffer()) {
            module.brightness = 7;
            module.scanLimit = 7;
            module.decodeMode = 255;
            module.hasValue = false;
            module.value = 0;
            module.buffer.fill(0x0f);
        }
        this->setClean();
    }

    void doSendBrightness() {
        for (unsigned module = 0; module < this->buffer().size(); ++module) {
            send(module, protocols::Max7219Command::SetBrightness, this->buffer()[module].brightness);
        }
        this->resetDirtyBrightness();
    }

    void doSendScanLimit() {
        for (unsigned module = 0; module < this->buffer().size(); ++module) {
            send(module, protocols::Max7219Command::SetScanLimit, this->buffer()[module].scanLimit);
        }
        this->resetDirtyScanLimit();
    }

    void doSendDecodeMode() {
        for (unsigned module = 0; module < this->buffer().size(); ++module) {
            send(module, protocols::Max7219Command::SetDecodeMode, this->buffer()[module].decodeMode);
        }
        this->resetDirtyDecodeMode();
    }

    /**
     * Send what is on each module: a number, or "blank". (Anything else, like segments set with setBuffer(), can not be
     * sent in a message.)
     */
    void doSendBuffer() {
        if (!this->isDirtyBuffer()) {
            return;
        }
        for (unsigned module = 0; module < this->buffer().size(); ++module) {
            const auto& state = this->buffer()[module];
            if (state.hasValue) {
                send(module, protocols::Max7219Command::SetValue, state.value);
            } else {
                send(module, protocols::Max7219Command::ClearDisplay);
            }
        }
        this->resetDirtyBuffer();
    }
};

} // namespace nl::rakis::raspberrypi::devices
