#pragma once
// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47
#include <numbers>
#include <cstdint>

namespace nl::rakis::raspberrypi::protocols {

enum class Commands : uint8_t {
    Hello       = 0x00,
    SetAddress  = 0x01,
    Enumerate   = 0x02,
    DeviceInfo  = 0x03,

    Error       = 0x0f,

    // Device specific commands
    Led         = 0x10,
    Max7219     = 0x11,
};

inline Commands toCommand(uint8_t value) {
    return static_cast<Commands>(value);
}

inline constexpr uint64_t ControllerId = 0x0000000000000000;

inline constexpr unsigned idSize = sizeof(uint64_t);

/**
 * @brief Board identifier
 */
union BoardId {
    uint64_t id;
    uint8_t bytes[idSize];
};

/**
 * @brief Broadcast message announcing a new controller.
 */
struct MsgHello {
    BoardId boardId;
};

/**
 * @brief A board manager's reply to set a controller's address.
 */
struct MsgSetAddress {
    BoardId boardId;
    uint8_t address;
};

/**
 * @brief A controller's request to enumerate attached interfaces and devices. 
*/
struct MsgEnumerate {
    uint8_t address;
};

struct MsgEnumerateReply {
    uint8_t numberOfInterfaces;
    uint8_t numberOfDevices;
};

/**
 * @brief A device's reply to an enumeration request.
 */
struct MsgDeviceInfo {
    uint8_t deviceId;
    uint8_t deviceType;
    uint8_t deviceSubType;
    uint8_t interfaceId;
    uint8_t deviceDetails;      // On I2C: Address, on SPI: Number of daisy-chained devices
    uint8_t numberOfPins;
    uint8_t pins[8];
};
enum class DeviceTypes : uint8_t {
    Led                 = 0x01,
    RgbLed              = 0x02,
    Max7219             = 0x20,
    LCD16x2             = 0x21,
};
enum class LedSubTypes : uint8_t {
    LedRed              = 0x01,
    LedGreen            = 0x02,
    LedBlue             = 0x03,
    LedYellow           = 0x04,
};
enum class RgbLedSubTypes : uint8_t {
    CommonAnode         = 0x01,
    CommonCathode       = 0x02,
};

struct MsgInterfaceInfo {
    uint8_t id;
    uint8_t type;
    uint8_t subType;
    uint8_t passthroughId; // For I/O expanders, the interface they are connected to
    uint8_t numberOfPins;
    uint8_t pins[8];
};
enum class InterfaceTypes : uint8_t {
    I2C                 = 0x01,
    SPI                 = 0x02,
    GPIO                = 0x03,
    PWM                 = 0x04,

    MCP23017            = 0x10,
    TLC59711            = 0x11,
};

enum class LedCommand : uint8_t {
    Off                 = 0x00,
    On                  = 0x01,
    Blink               = 0x02,
    Pulse               = 0x03,
};

struct MsgLed {
    uint8_t deviceId;
    LedCommand command;
};


enum class Max7219Command : uint8_t {
    Reset               = 0x00,
    SetNumModules       = 0x01,
    ClearDisplay        = 0x02,
    SetBrightness       = 0x03,
    SetValue            = 0x04,
    SendData            = 0x05,
};

struct MsgMax7219 {
    uint8_t deviceId;
};
struct MsgMax7219Reset : MsgMax7219 {
};

} // namespace nl::rakis::raspberrypi::protocols