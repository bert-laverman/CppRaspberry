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