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


#include <chrono>
#include <cstring>
#include <string>
#include <format>
#include <exception>

extern "C" {
#include <pigpiod_if2.h>
}

#include <interfaces/pigpiod-i2c.hpp>


using namespace nl::rakis::raspberrypi::interfaces;
namespace protocols = nl::rakis::raspberrypi::protocols;


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

/**
 * The largest payload we accept in a message, and the highest valid 7-bit address for a sender. A header that
 * claims more than this, or one whose payload does not match its checksum, is not a message.
 */
static constexpr size_t maxPayload{ 64 };

/**
 * A message is sent in one go, which takes about a millisecond at 100 kHz. If the rest of a partial message has not
 * arrived after this long, it never will, and what is left must not be mixed up with the next message.
 */
static constexpr std::chrono::milliseconds maxStale{ 20 };

/**
 * Only the commands we know are accepted. Together with the checksum this keeps noise from passing as a message,
 * for instance a payload of length 0, which has checksum 0.
 */
static bool isKnownCommand(uint8_t command)
{
    switch (protocols::toCommand(command)) {
    case protocols::Command::Hello:
    case protocols::Command::SetAddress:
    case protocols::Command::Enumerate:
    case protocols::Command::InterfaceInfo:
    case protocols::Command::DeviceInfo:
    case protocols::Command::Log:
    case protocols::Command::Led:
    case protocols::Command::Max7219:
    case protocols::Command::Button:
        return true;
    default:
        return false;
    }
}

/**
 * A sender is either "no address yet" (0x00), or a valid 7-bit address. (0x01-0x07 and 0x78-0x7f are reserved.)
 */
static bool isValidSender(uint8_t sender)
{
    return (sender == 0x00) || ((sender >= 0x08) && (sender <= 0x77));
}

static uint8_t checksumOf(const uint8_t* data, size_t size)
{
    uint8_t checksum{ 0 };
    for (size_t i = 0; i < size; ++i) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * Collect the bytes that come in from the bus, and hand over every complete message.
 *
 * Bytes can arrive in pieces that do not line up with messages, and a disturbed transmission (for example two
 * masters that start at the same time) can leave extra or missing bytes. So a message is only accepted if its header
 * is plausible and the checksum of its payload matches. If not, one byte is dropped and the search for a valid
 * header continues, so the stream recovers by itself.
 */
void PigpiodBSCI2C::processBytes(std::span<uint8_t> data)
{
    const auto now = std::chrono::steady_clock::now();
    if (!bytes_.empty() && ((now - lastReceived_) > maxStale)) {
        if (verbose()) { log(std::format("Discarding {} stale byte(s) of an incomplete message.", bytes_.size())); }
        bytes_.clear();
    }
    lastReceived_ = now;

    bytes_.insert(bytes_.end(), data.begin(), data.end());
    if (verbose()) {
        std::string hex;
        for (auto byte : data) {
            hex += std::format("{:02x} ", byte);
        }
        log(std::format("Received {} bytes, now {} in buffer: {}", data.size(), bytes_.size(), hex));
    }

    while (bytes_.size() >= protocols::MsgHeaderSize) {
        protocols::MsgHeader header;
        std::memcpy(&header, bytes_.data(), protocols::MsgHeaderSize);

        if ((header.length > maxPayload) || !isValidSender(header.sender) || !isKnownCommand(header.command)) {
            if (verbose()) { log(std::format("Dropping byte 0x{:02x}: not a valid header.", bytes_.front())); }
            bytes_.erase(bytes_.begin());
            continue;
        }
        if (bytes_.size() < protocols::MsgHeaderSize + header.length) {
            break;      // the rest of the message has not arrived yet
        }
        const uint8_t* payload = bytes_.data() + protocols::MsgHeaderSize;
        if (checksumOf(payload, header.length) != header.checksum) {
            if (verbose()) { log(std::format("Dropping byte 0x{:02x}: checksum does not match.", bytes_.front())); }
            bytes_.erase(bytes_.begin());
            continue;
        }

        if (callback()) {
            callback()(protocols::toCommand(header.command), header.sender, std::span<uint8_t>(bytes_.data() + protocols::MsgHeaderSize, header.length));
        } else {
            log(std::format("Received message from 0x{:02x} with command 0x{:02x} and length {}, but no callback", header.sender, header.command, header.length));
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
            // The BSC receive FIFO holds just 16 bytes, so poll often enough to empty it while a message is still coming in.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
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

    // The BSC slave stays as it was if the program that used it was killed in the middle of a transfer, and then it can keep
    // SCL low. Switch it off first, so we start from a clean slate.
    {
        bsc_xfer_t off;
        std::memset(&off, 0, sizeof(off));
        bsc_i2c(channel(), 0, &off);
    }

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
        log("Told pigpiod to stop listening");
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