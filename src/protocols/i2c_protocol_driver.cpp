// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47

#include <protocols/i2c_protocol_driver.hpp>

namespace nl::rakis::raspberrypi::protocols {

template <typename I2CImpl>
void I2CProtocolDriver<I2CImpl>::messageReceived(uint8_t address, std::vector<uint8_t> const& data)
{
    if (data.size() < MsgHeaderSize) {
        log() << "Message too short: " << data.size() << " bytes\n";
        return;
    }

    MsgHeader header;
    std::memcpy(&header, data.data(), MsgHeaderSize);

    if (std::memcmp(header.rakisMagic, "RAKISPI", 7) != 0) {
        log() << "Invalid magic\n";
        return;
    }

    if (header.sender == address) {
        log() << "Ignoring message from self\n";
        return;
    }

    if (header.length != data.size() - MsgHeaderSize) {
        log() << "Invalid length\n";
        return;
    }

    uint8_t checksum = 0;
    for (size_t i = 0; i < header.length; ++i) {
        checksum += data[MsgHeaderSize + i];
    }

    if (checksum != header.checksum) {
        log() << "Invalid checksum\n";
        return;
    }

    switch (header.command) {
    case static_cast<uint8_t>(Commands::Hello):
        if (header.length != sizeof(MsgHello)) {
            log() << "Invalid Hello message length\n";
            return;
        }
        MsgHello hello;
        std::memcpy(&hello, data.data() + MsgHeaderSize, sizeof(MsgHello));
        log() << "Hello from board " << hello.boardId.id << '\n';
        break;

    case static_cast<uint8_t>(Commands::SetAddress):
        if (header.length != sizeof(MsgSetAddress)) {
            log() << "Invalid SetAddress message length\n";
            return;
        }
        MsgSetAddress setAddress;
        std::memcpy(&setAddress, data.data() + MsgHeaderSize, sizeof(MsgSetAddress));
        log() << "Set address " << setAddress.address << " for board " << setAddress.boardId.id << '\n';
        break;

    default:
        log() << "Unknown command " << std::hex << static_cast<uint16_t>(header.command) << std::dec << '\n';
        break;
    }
}

} // namespace nl::rakis::raspberrypi::protocols