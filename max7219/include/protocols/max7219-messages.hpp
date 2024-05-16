#pragma once
// Copyright (c) 2024 by Bert Laverman, all rights reserved.

#include <protocols/i2c_protocol.hpp>

namespace nl::rakis::raspberrypi::protocols {

enum class Max7219Command : uint8_t {
    Reset               = 0x00,
    Shutdown            = 0x01,
    Startup             = 0x02,
    TestDisplay         = 0x03,

    SetBrightness       = 0x08,
    SetScanLimit        = 0x09,
    SetDecodeMode       = 0x0a,
    SetSendImmediately  = 0x0b,

    ClearDisplay        = 0x10,
    SetValue            = 0x11,

    SendBrightness      = 0x20,
    SendScanLimit       = 0x21,
    SendDecodeMode      = 0x22,
    SendBuffer          = 0x23,
};

consteval Max7219Command toMax7219Command(uint8_t value) noexcept { return static_cast<Max7219Command>(value); }
consteval uint8_t toValue(Max7219Command cmd) noexcept { return static_cast<uint8_t>(cmd); }

struct MsgMax7219 {
    uint8_t interfaceId;
    uint8_t module;
    uint8_t command;
    uint32_t value;

    inline constexpr static uint8_t AllModules = 0xff;
};


} // namespace nl::rakis::raspberrypi::protocols