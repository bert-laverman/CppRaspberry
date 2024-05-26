#pragma once
// Copyright (c) 2024 by Bert Laverman, all rights reserved.

#include <protocols/messages.hpp>

namespace nl::rakis::raspberrypi::protocols {

enum class Max7219Command : uint8_t {
    Reset               = 0x00,
    Shutdown            = 0x01,
    Startup             = 0x02,
    TestDisplay         = 0x03,

    SetBrightness       = 0x08,
    SetScanLimit        = 0x09,
    SetDecodeMode       = 0x0a,

    ClearDisplay        = 0x10,
    SetValue            = 0x11,

    SetSendImmediately  = 0x20,
    SendBrightness      = 0x21,
    SendScanLimit       = 0x22,
    SendDecodeMode      = 0x23,
    SendBuffer          = 0x24,
    SendData            = 0x25,
};

inline Max7219Command toMax7219Command(uint8_t value) noexcept { return static_cast<Max7219Command>(value); }
inline uint8_t toValue(Max7219Command cmd) noexcept { return static_cast<uint8_t>(cmd); }

struct MsgMax7219 {
    uint8_t interfaceId{ 0 };
    uint8_t module{ AllModules };
    uint8_t command;
    int32_t value { 0 };

    inline constexpr static uint8_t AllModules = 0xff;
};


} // namespace nl::rakis::raspberrypi::protocols