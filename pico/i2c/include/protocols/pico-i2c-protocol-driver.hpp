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


#include <list>
#include <tuple>
#include <vector>

#if !defined(TARGET_PICO)
#error "The PicoI2CProtocolDriver runs only on the Raspberry Pi Pico"
#endif
#if !defined(HAVE_I2C)
#error "The PicoI2CProtocolDriver requires I2C support"
#endif

#include <pico/sync.h>


#include <interfaces/pico-i2c.hpp>
#include <protocols/messages.hpp>
#include <protocols/i2c-protocol-driver.hpp>


namespace nl::rakis::raspberrypi::protocols {

class PicoI2CProtocolDriver : public I2CProtocolDriver<interfaces::PicoI2C, interfaces::PicoI2C>
{
    std::list<std::tuple<protocols::Command,uint8_t,std::vector<uint8_t>>> incoming_;
    critical_section_t section_;

public:
    PicoI2CProtocolDriver(interfaces::PicoI2C& ic2Out, interfaces::PicoI2C& i2cIn) : I2CProtocolDriver(ic2Out, i2cIn) {}

    virtual ~PicoI2CProtocolDriver() { critical_section_init(&section_); }
    PicoI2CProtocolDriver(const PicoI2CProtocolDriver&) = delete;
    PicoI2CProtocolDriver(PicoI2CProtocolDriver&&) = delete;
    PicoI2CProtocolDriver& operator=(const PicoI2CProtocolDriver&) = delete;
    PicoI2CProtocolDriver& operator=(PicoI2CProtocolDriver&&) = delete;

    virtual void enableResponderMode(uint8_t address) override {
        i2cIn().switchToResponderMode(address, [this](Command command, uint8_t sender, const std::span<uint8_t> data) {
            incoming_.push_back({command, sender, std::vector<uint8_t>(data.begin(), data.end())});
        });
    }

    bool haveMessages() {
        critical_section_enter_blocking(&section_);
        auto result = !incoming_.empty();
        critical_section_exit(&section_);
        return result;
    }

    bool popMessage(Command& command, uint8_t& sender, std::vector<uint8_t>& data) {
        auto result = !incoming_.empty();
        bool error{ false };

        critical_section_enter_blocking(&section_);
        try {
            if (result) {
                command = std::get<0>(incoming_.front());
                sender = std::get<1>(incoming_.front());
                data = std::move(std::get<2>(incoming_.front()));
                incoming_.pop_front();
            }
        }
        catch (...) {
            error = true;
        }
        critical_section_exit(&section_);
        if (error) {
            printf("Error trying to remove a message from the 'incoming' queue.");
            result = false;
        }
        return result;
    }
};

} // namespace nl::rakis::raspberrypi::protocols