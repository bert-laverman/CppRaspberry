#pragma once
// Copyright 2024 by Bert Laverman, All rights reserved.

#include <cstdint>

#include <span>
#include <functional>


namespace nl::rakis::raspberrypi::protocols {


/**
 * @brief This enumeration defines the commands (or message types) we can send.)
 */
enum class Command : uint8_t {
    /**
     * @brief The "Hello" message is a General Call that announces the presence of a listener. If the announced address is 0x00, which
     *        (conveniently) is also the I2C General Call address, the listener is announcing it does not have an address yet and will
     *        only listener for a "SetAddress" General Call. The message body is a (64 bit) BoardId, with BoardId 0x0000_0000_0000_0000
     *        being the bus controller.
     */
    Hello       = 0x00,

    /**
     * @brief The "SetAddress" message is only sent as a General Call and instructs a listener to listen using the provided address.
     */
    SetAddress  = 0x01,

    /**
     * @brief the "Enumerate" message requests an enumeration of attached interfaces and devices.
     *
     * NOT IMPLEMENTED YET!
     */
    Enumerate   = 0x02,

    /**
     * @brief The "InterfaceInfo" message contains data on attached interfaces, and is used to respond to an "Enumerate"
     *        request, or to provide a listener with information about attached interfaces.
     *
     * NOT IMPLEMENTED YET!
     */
    InterfaceInfo  = 0x03,

    /**
     * @brief The "DeviceInfo" message contains data on attached devices, and is used to respond to an "Enumerate"
     *        request, or to provide a listener with information about attached devices.
     *
     * NOT IMPLEMENTED YET!
     */
    DeviceInfo  = 0x04,

    /**
     * @brief Devices can inform the bus controller of important events using "Log" messages.
     */
    Log         = 0x0f,

    // Device specific commands

    /**
     * @brief Messages using to control attached Leds.
     */
    Led         = 0x10,

    /**
     * @brief Messages used to control attached Numeric displays using the Max7219 controller.
     */
    Max7219     = 0x11,

    /**
     * @brief Messages using to report button, rotary encoder, and switch state changes.
     */
    Button      = 0x012,
};


inline constexpr Command toCommand(uint8_t value) {
    return static_cast<Command>(value);
}
inline constexpr uint8_t toInt(Command value) {
    return static_cast<uint8_t>(value);
}


using MsgCallback = std::function<void(Command command, uint8_t sender, const std::span<uint8_t> data)>;

/**
 * @brief The "Bus Controller" uses the magic BoardId value 0. (zero)
 */
inline constexpr uint64_t ControllerId = 0x0000000000000000;


/**
 * @brief A BoardId uses a 64-bit value, so 8 bytes.
 */
inline constexpr unsigned idSize = sizeof(uint64_t);

/**
 * @brief Board identifier, a unique id for devices on the bus.
 */
union BoardId {
    uint64_t id;
    uint8_t bytes[idSize];
};

/**
 * @brief Broadcast (aka I2C General Call) message announcing a device on the bus.
 */
struct MsgHello {
    BoardId boardId;
};


/**
 * @brief A bus controller's broadcast (aka I2C General Call) message to set a listener's address.
 */
struct MsgSetAddress {
    BoardId boardId;
    uint8_t address;
};


/**
 * @brief A request to enumerate attached devices for all interfaces (default) or a specific interface.
*/
struct MsgEnumerate {
    bool includeInterfaces{ true };
    bool includeDevices{ true };
    uint8_t interface{ 0x00 };
};


/**
 * @brief A message detailing information about attached interfaces. This can be a response to an enumeration request
 *        or information pushed to a listener.
 */
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

/**
 * @brief A message detailing information about attached devices. This can be a response to an enumeration request
 *        or information pushed to a listener.
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

} // namespace nl::rakis::raspberrypi::protocols