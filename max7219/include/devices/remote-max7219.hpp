#pragma once
// Copyright 2024 (c) Bert Laverman, all rights reserved

#include <cstdint>

#include <devices/max7219.hpp>

#include <protocols/i2c_protocol_driver.hpp>
#include <protocols/max7219-messages.hpp>


using namespace nl::rakis::raspberrypi::protocols;

namespace nl::rakis::raspberrypi::devices {

template <typename I2COutImpl, typename I2CInImpl>
class RemoteMax7219 : public MAX7219 {
    I2CProtocolDriver<I2COutImpl, I2CInImpl>& driver_;
    uint8_t address_;

public:
    RemoteMax7219(protocols::I2CProtocolDriver<I2COutImpl, I2CInImpl>& driver, uint8_t address) : driver_(driver), address_(address) {}
    virtual ~RemoteMax7219() = default;

    void sendBrightness() override {
        MsgMax7219 msg{ .interfaceId =0x00, .module =MsgMax7219::AllModules, .command = toValue(Max7219Command::SendBrightness) };
        driver_.sendMessage(address_, Commands::Max7219, msg);
    }

    void sendScanLimit() override {
        MsgMax7219 msg{ .interfaceId =0x00, .module =MsgMax7219::AllModules, .command = toValue(Max7219Command::SendScanLimit) };
        driver_.sendMessage(address_, Commands::Max7219, msg);
    }

    void sendDecodeMode() override {
        MsgMax7219 msg{ .interfaceId =0x00, .module =MsgMax7219::AllModules, .command = toValue(Max7219Command::SendDecodeMode) };
        driver_.sendMessage(address_, Commands::Max7219, msg);
    }

    void sendBuffer() override {
        MsgMax7219 msg{ .interfaceId =0x00, .module =MsgMax7219::AllModules, .command = toValue(Max7219Command::SendBuffer) };
        driver_.sendMessage(address_, Commands::Max7219, msg);
    }

    void shutdown() override {
        MsgMax7219 msg{ .interfaceId =0x00, .module =MsgMax7219::AllModules, .command = toValue(Max7219Command::Shutdown) };
        driver_.sendMessage(address_, Commands::Max7219, msg);
    }

    void shutdown(uint8_t module) override {
        MsgMax7219 msg{ .interfaceId =0x00, .module =module, .command = toValue(Max7219Command::Shutdown) };
        driver_.sendMessage(address_, Commands::Max7219, msg);
    }

    void startup() override {
        MsgMax7219 msg{ .interfaceId =0x00, .module =MsgMax7219::AllModules, .command = toValue(Max7219Command::Startup) };
        driver_.sendMessage(address_, Commands::Max7219, msg);
    }

    void startup(uint8_t module) override {
        MsgMax7219 msg{ .interfaceId =0x00, .module =module, .command = toValue(Max7219Command::Startup) };
        driver_.sendMessage(address_, Commands::Max7219, msg);
    }

    void displayTest(uint8_t value) override {
        MsgMax7219 msg{ .interfaceId =0x00, .module =MsgMax7219::AllModules, .command = toValue(Max7219Command::TestDisplay), .value = value };
        driver_.sendMessage(address_, Commands::Max7219, msg);
    }

    void displayTest(uint8_t module, uint8_t value) override {
        MsgMax7219 msg{ .interfaceId =0x00, .module =module, .command = toValue(Max7219Command::TestDisplay), .value = value };
        driver_.sendMessage(address_, Commands::Max7219, msg);
    }

    void clear() override {
        MsgMax7219 msg{ .interfaceId =0x00, .module =MsgMax7219::AllModules, .command = toValue(Max7219Command::ClearDisplay) };
        driver_.sendMessage(address_, Commands::Max7219, msg);
    }

    void clear(uint8_t module) override {
        MsgMax7219 msg{ .interfaceId =0x00, .module =module, .command = toValue(Max7219Command::ClearDisplay) };
        driver_.sendMessage(address_, Commands::Max7219, msg);
    }

    void setNumber(uint8_t module, uint32_t value) override {
        MsgMax7219 msg{ .interfaceId =0x00, .module =module, .command = toValue(Max7219Command::SetValue), .value = value };
        driver_.sendMessage(address_, Commands::Max7219, msg);
    }

    void reset() override {
        MsgMax7219 msg{ .interfaceId =0x00, .module =MsgMax7219::AllModules, .command = toValue(Max7219Command::Reset) };
        driver_.sendMessage(address_, Commands::Max7219, msg);
    }

};

} // namespace nl::rakis::raspberrypi::devices