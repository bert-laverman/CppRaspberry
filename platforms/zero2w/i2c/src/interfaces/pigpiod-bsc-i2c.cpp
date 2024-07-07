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


#include <cstring>
#include <format>
#include <exception>

extern "C" {
#include <pigpiod_if2.h>
}

#include <interfaces/pigpiod-i2c.hpp>


using namespace nl::rakis::raspberrypi::interfaces;


enum class PIGPIO_Control : uint32_t {
    InvertTransmitStatus = 0x2000,
    EnableHostControl = 0x1000,
    EnableTestMode = 0x0800,
    InvertReceiveStatus = 0x0400,
    EnableReceive = 0x0200,
    EnableTransmit = 0x0100,
    AbortAndClearFIFO = 0x0080,
    SendControlRegister = 0x0040,
    SendStatusRegister = 0x0020,
    SetSPIPolarityHigh = 0x0010,
    SetSPIPhaseHigh = 0x0008,
    EnableI2C = 0x0004,
    EnableSPI = 0x0002,
    EnableBS1 = 0x0001
};

static consteval PIGPIO_Control operator|(PIGPIO_Control a, PIGPIO_Control b) {
    return static_cast<PIGPIO_Control>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
static uint32_t Control(uint8_t address, PIGPIO_Control control) {
    return (static_cast<uint32_t>(address) << 16) | static_cast<uint32_t>(control);
}


PigpiodBSCI2C::~PigpiodBSCI2C()
{
    close();
}

void PigpiodBSCI2C::open()
{
    if (initialized()) {
        return;
    }
    log("Opening channel to pigpiod.");
    channel(pigpio_start(nullptr, nullptr));
    if (channel() < 0) {
        log(std::format("Failed to open I2C channel: {}", channel()));

        return;
    }
    initialized(true);
}

bool PigpiodBSCI2C::canListen() const noexcept {
    return true;
}

void PigpiodBSCI2C::processBytes(std::span<uint8_t> data)
{
    bytes_.insert(bytes_.end(), data.begin(), data.end());
    while (bytes_.size() >= protocols::MsgHeaderSize) {
        protocols::MsgHeader header;
        std::memcpy(&header, bytes_.data(), protocols::MsgHeaderSize);
        if (bytes_.size() < protocols::MsgHeaderSize + header.length) {
            break;
        }
        if (callback()) {
            callback()(protocols::toCommand(header.command), header.sender, std::span<uint8_t>(bytes_.data() + protocols::MsgHeaderSize, header.length));
        }
        bytes_.erase(bytes_.begin(), bytes_.begin() + protocols::MsgHeaderSize + header.length);
    }
}

void PigpiodBSCI2C::listen(PigpiodBSCI2C& bus)
{
    if (!bus.initialized() || !bus.listening()) {
        bus.log(std::format("listen(): Not initialized ({}) or not listening ({})", !bus.initialized(), !bus.listening()));

        return;
    }
    bus.log(std::format("Listening on channel {} and address 0x{:02x}", bus.channel(), bus.listenAddress()));

    bsc_xfer_t xfer;
    std::memset(&xfer, 0, sizeof(xfer));
    xfer.control = Control(bus.listenAddress(), PIGPIO_Control::EnableTransmit | PIGPIO_Control::EnableReceive | PIGPIO_Control::EnableI2C | PIGPIO_Control::EnableBS1);

    while (bus.listening()) {
        int status = bsc_i2c(bus.channel(), bus.listenAddress(), &xfer);
        if (status < 0) {
            bus.log(std::format("bsc_i2c() returned error {}", status));
        } else if ((status > 0) && (xfer.rxCnt > 0)) {
            bus.processBytes(std::span<uint8_t>(reinterpret_cast<uint8_t*>(xfer.rxBuf), xfer.rxCnt));
        }
        if  (xfer.rxCnt == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    bus.log("Listener thread stopped");
}

void PigpiodBSCI2C::startListening()
{
    open();

    if (!initialized() || listening()) {
        log(std::format("startListening(): Not initialized ({}), or already listening, so ok. ({})", !initialized(), listening()));

        return;
    }
    log(std::format("Start listening on channel {} and address 0x{:02x}", channel(), listenAddress()));

    bsc_xfer_t xfer;

    std::memset(&xfer, 0, sizeof(xfer));
    xfer.control = Control(listenAddress(), PIGPIO_Control::EnableTransmit | PIGPIO_Control::EnableReceive | PIGPIO_Control::EnableI2C | PIGPIO_Control::EnableBS1);

    int status = bsc_i2c(channel(), listenAddress(), &xfer);
    if (verbose()) {
        if (status < 0) {
            log(std::format("Initial call failed with {}", status & 0x0000ffff));
        } else if (status > 0) {
            log(std::format("Initial call returned 0x{:04x}", status));
        }
    }
    if (xfer.rxCnt > 0) {
        processBytes(std::span<uint8_t>(reinterpret_cast<uint8_t*>(xfer.rxBuf), xfer.rxCnt));
    }
    listening(true);
    listener_ = std::jthread([this]() { listen(*this); });
}

void PigpiodBSCI2C::stopListening()
{
    if (!initialized() || !listening()) {
        log(std::format("stopListening(): Not initialized ({}) or not listening, so ok. ({})", !initialized(), !listening()));

        return;
    }
    log(std::format("Stop listening on channel {} and address 0x{:02x}", channel(), listenAddress()));

    listening(false);
    listener_.join();

    log("Listener thread joined");

    bsc_xfer_t xfer;
    std::memset(&xfer, 0, sizeof(xfer));

    int status = bsc_i2c(channel(), 0, &xfer);
    if (verbose()) {
        if (status < 0) {
            log(std::format("bsc_i2c() returned error {}", status));
        } else if (status > 0) {
            log(std::format("bsc_i2c() returned 0x{:04x}", status));
        }
        std::cerr << "Told pigpiod to stop listening";
    }
}

void PigpiodBSCI2C::close() {
    if (!initialized()) {
        return;
    }
    stopListening();

    if (channel() >= 0) {
        log(std::format("Closing channel {}", channel()));
        pigpio_stop(channel());
        channel(-1);
    }
    initialized(false);
}

bool PigpiodBSCI2C::canSend() const noexcept {
    return false;
}

bool PigpiodBSCI2C::write([[maybe_unused]] uint8_t address, [[maybe_unused]]std::span<uint8_t> data)
{
    throw std::runtime_error("PigpiodBSCI2C can only function as Listener.");
}