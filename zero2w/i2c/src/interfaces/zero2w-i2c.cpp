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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

extern "C" {

#include <i2c/smbus.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <pigpiod_if2.h>
}

#include <interfaces/zero2w-i2c.hpp>

/*
 * The I2C interface on the non-Pico Raspberry Pi's is a mess. Most libraries provide the Controller side of the protocol,
 * but not the Listener side. The pigpio library does provide the Listener side, but that is not compatible with its Controller
 * implementation. It even uses different pins for the SDA and SCL lines: The Controller side uses GPIO pins 2 and 3 for the
 * first (not zeroeth) I2C bus, while the Listener side uses GPIO pins 18 and 19. This is because it uses the hardware I2C
 * support on the Broadcom SoC, which is somehow also tied to the SPI support, and uses the PCM_FS and PCM_CLK pins.
 *
 * Note that this implementation uses pigpiod_if2.h, which is the C interface to the pigpiod daemon. This daemon must be
 * installed and running so we won't have to run as root. The pigpio library is a C library, so we have to use the C interface.
 *
 * For the Controller side I chose i2c-dev, which is the standard Linux I2C interface. This is a bit of a kludge, but it works.
 */

using nl::rakis::raspberrypi::protocols::MsgHeader;
using nl::rakis::raspberrypi::protocols::MsgHeaderSize;
using namespace nl::rakis::raspberrypi::interfaces;

/*
 * The Zero2WI2C_i2cdev class is an implementation of the Zero2WI2C interface using the i2c-dev library.
 * It only provide the I2C Controller functionality, using the ioctl() calls.
 */


Zero2WI2C_i2cdev::~Zero2WI2C_i2cdev()
{
    close();
}

/**
 * Opens the I2C interface specified by `interface_`.
 *
 * If the interface is already open, this function simply returns.
 * Otherwise, it attempts to open the interface using `::open()`. If the
 * open operation is successful, the `initialized()` flag is set to `true`.
 * If the open operation fails, the `fd_` member is set to `-1` and the
 * `initialized()` flag is set to `false`.
 *
 * This function is used internally by the `Zero2WI2C_i2cdev` class to
 * manage the lifetime of the I2C interface.
 */
void Zero2WI2C_i2cdev::open() {
  if (fd_ >= 0) {
    return;
  }

  if (verbose()) {
    log() << "Opening '" << interface_ << "'\n";
  }
  fd_ = ::open(interface_.c_str(), O_RDWR);
  if (fd_ < 0) {
    if (verbose()) {
      log() << "Failed to open '" << interface_ << "'. Errno=" << errno
            << ".\n";
    }
  } else {
    initialized(true);
  }
}

void Zero2WI2C_i2cdev::close() {
    if (fd_ >= 0) {
        if (verbose()) {
            log() << "Closing '" << interface_ << "'\n";
        }
        ::close(fd_);
        fd_ = -1;
        initialized(false);
    }
}

void Zero2WI2C_i2cdev::switchToControllerMode()
{
    if (verbose()) {
        log() << "Switching to Controller mode\n";
    }
    reset();
}

void Zero2WI2C_i2cdev::switchToResponderMode([[maybe_unused]] uint8_t address, [[maybe_unused]] protocols::MsgCallback cb)
{
    log() << "Responder mode not available via i2c-dev\n";
    throw std::runtime_error("Responder mode not available via i2c-dev");
}

bool Zero2WI2C_i2cdev::readBytes([[maybe_unused]] uint8_t address, [[maybe_unused]] std::span<uint8_t> data)
{
    log() << "readBytes() not implemented\n";
    throw std::runtime_error("readBytes() not implemented");
}

bool Zero2WI2C_i2cdev::writeBytes(uint8_t address, std::span<uint8_t> data)
{
    open();

    log() << std::format("Going to send {} bytes to 0x{:02x}.\n", data.size(), address);
    struct i2c_msg msg{ address, 0, static_cast<__u16>(data.size()), data.data() };
    struct i2c_rdwr_ioctl_data msgs{ &msg, 1 };

    auto result = ::ioctl(fd_, I2C_RDWR, &msgs);
    if (result == 0) {
        return true;
    }
    if (result != 1) {
        log() << std::format("Failed to write {} bytes to 0x{:02x}. Errno={}.\n", data.size(), address, errno);
        return false;
    }
    return true;
}


/*
 * The Zero2WI2C_pigpio class is an implementation of the Zero2WI2C interface using the pigpio library.
 * It only provides the I2C Listener functionality, using the bsc_i2c() call.
 */

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

inline consteval PIGPIO_Control operator|(PIGPIO_Control a, PIGPIO_Control b) {
    return static_cast<PIGPIO_Control>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline uint32_t Control(uint8_t address, PIGPIO_Control control) {
    return (static_cast<uint32_t>(address) << 16) | static_cast<uint32_t>(control);
}

Zero2WI2C_pigpio::~Zero2WI2C_pigpio()
{
    close();
}

void Zero2WI2C_pigpio::open()
{
    if (initialized()) {
        log() << "open(): Already initialized\n";
        return;
    }
    // if (verbose()) {
        log() << "Opening I2C channel\n";
    // }
    channel(pigpio_start(nullptr, nullptr));
    if (channel() < 0) {
        log() << std::format("Failed to open I2C channel: {}\n", channel());
        return;
    }
    initialized(true);
}

void Zero2WI2C_pigpio::processBytes(std::span<uint8_t> data)
{
    bytes_.insert(bytes_.end(), data.begin(), data.end());
    while (bytes_.size() >= MsgHeaderSize) {
        MsgHeader header;
        std::memcpy(&header, bytes_.data(), MsgHeaderSize);
        if (bytes_.size() < MsgHeaderSize + header.length) {
            break;
        }
        if (callback()) {
            callback()(protocols::toCommand(header.command), header.sender, std::span<uint8_t>(bytes_.data() + MsgHeaderSize, header.length));
        }
        bytes_.erase(bytes_.begin(), bytes_.begin() + MsgHeaderSize + header.length);
    }
}

void Zero2WI2C_pigpio::listen(Zero2WI2C_pigpio& bus)
{
    if (!bus.initialized() || !bus.listening()) {
        std::cerr << std::format("listen(): Not initialized ({}) or not listening ({})\n", !bus.initialized(), !bus.listening());
        return;
    }
    // if (verbose()) {
        std::cerr << std::format("Listening on channel {} and address 0x{:02x}\n", bus.channel(), bus.listenAddress());
    // }
    bsc_xfer_t xfer;
    std::memset(&xfer, 0, sizeof(xfer));
    xfer.control = Control(bus.listenAddress(), PIGPIO_Control::EnableTransmit | PIGPIO_Control::EnableReceive | PIGPIO_Control::EnableI2C | PIGPIO_Control::EnableBS1);

    while (bus.listening()) {
        int status = bsc_i2c(bus.channel(), bus.listenAddress(), &xfer);
        if (status < 0) {
            std::cerr << std::format("bsc_i2c() returned error {}\n", status);
        } else if ((status > 0) && (xfer.rxCnt > 0)) {
            bus.processBytes(std::span<uint8_t>(reinterpret_cast<uint8_t*>(xfer.rxBuf), xfer.rxCnt));
        }
        if  (xfer.rxCnt == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    std::cerr << "Listener thread stopped\n";
}

void Zero2WI2C_pigpio::startListening()
{
    if (!initialized() || listening()) {
        log() << std::format("startListening(): Not initialized ({}), or already listening, so ok. ({})\n", !initialized(), listening());
        return;
    }
    // if (verbose()) {
        log() << std::format("Start listening on channel {} and address 0x{:02x}\n", channel(), listenAddress());
    // }
    bsc_xfer_t xfer;

    std::memset(&xfer, 0, sizeof(xfer));
    xfer.control = Control(listenAddress(), PIGPIO_Control::EnableTransmit | PIGPIO_Control::EnableReceive | PIGPIO_Control::EnableI2C | PIGPIO_Control::EnableBS1);

    int status = bsc_i2c(channel(), listenAddress(), &xfer);
    if (status < 0) {
        log() << std::format("Initial call failed with {}\n", status & 0x0000ffff);
        // return;
    }
    if (status > 0) {
        log() << std::format("Initial call returned 0x{:04x}\n", status);
    }
    if (xfer.rxCnt > 0) {
        processBytes(std::span<uint8_t>(reinterpret_cast<uint8_t*>(xfer.rxBuf), xfer.rxCnt));
    }
    listening(true);
    listener_ = std::jthread([this]() { listen(*this); });
}

void Zero2WI2C_pigpio::stopListening()
{
    if (!initialized() || !listening()) {
        log() << std::format("stopListening(): Not initialized ({}) or not listening, so ok. ({})\n", !initialized(), !listening());
        return;
    }
    // if (verbose()) {
        log() << std::format("Stop listening on channel {} and address 0x{:02x}\n", channel(), listenAddress());
    // }
    listening(false);
    listener_.join();
    log() << "Listener thread joined\n";

    bsc_xfer_t xfer;
    std::memset(&xfer, 0, sizeof(xfer));

    int status = bsc_i2c(channel(), 0, &xfer);
    if (status < 0) {
        log() << std::format("bsc_i2c() returned error {}\n", status);
    } else if (status > 0) {
        log() << std::format("bsc_i2c() returned 0x{:04x}\n", status);
    }
    std::cerr << "Told pigpiod to stop listening\n";
}

void Zero2WI2C_pigpio::close() {
    if (!initialized()) {
        return;
    }
    stopListening();

    if (channel() >= 0) {
        // if (verbose()) {
            log() << std::format("Closing channel {}\n", channel());
        // }
        pigpio_stop(channel());
        channel(-1);
    }
    initialized(false);
}

bool Zero2WI2C_pigpio::writeBytes(uint8_t address, std::span<uint8_t> data)
{
    if (data.empty()) {
        log() << "writeBytes(): No data to write\n";
        return true;
    }
    int handle{ -1 };
    bool success{ false };
    try {
        constexpr int bus = 1;
        handle = i2c_open(channel(), bus, address, 0);
        if (handle < 0) {
            switch (handle) {
            case PI_BAD_I2C_ADDR:
                log() << std::format("Bad I2C address 0x{:02x}\n", address);
                break;
            case PI_BAD_I2C_BUS:
                log() << std::format("Bad I2C bus number {}\n", bus);
                break;
            case PI_BAD_FLAGS:
                log() << "Bad I2C flags\n";
                break;
            case PI_BAD_HANDLE:
                log() << "Bad I2C handle\n";
                break;
            case PI_BAD_PARAM:
                log() << "Bad I2C parameter\n";
                break;
            case PI_I2C_OPEN_FAILED:
                log() << "I2C open failed\n";
                break;
            case PI_NO_HANDLE:
                log() << "No I2C handle\n";
                break;
            default:
                log() << std::format("Failed to open I2C handle for address 0x{:02x}\n", address);
                break;
            }
        } else {
            auto result = i2c_write_device(channel(), handle, reinterpret_cast<char*>(data.data()), data.size());
            if (result < 0) {
                log() << std::format("Failed to write {} bytes to address 0x{:02x}\n", data.size(), address);
            } else {
                success = true;
            }
        }
    } catch (...) {
        log() << "Exception caught in writeBytes()\n";
    }
    if (handle >= 0) {
        i2c_close(channel(), handle);
    }

    return success;
}

bool Zero2WI2C_pigpio::readBytes([[maybe_unused]] uint8_t address, [[maybe_unused]] std::span<uint8_t> data)
{
    return false;
}

bool Zero2WI2C_pigpio::readMessage(MsgHeader &header, std::vector<uint8_t> &data)
{
    open();

    std::array<uint8_t, MsgHeaderSize> headerBytes;
    if (!readBytes(listenAddress(), headerBytes)) {
        return false;
    }

    header.command = headerBytes[0];
    header.length = headerBytes[1];
    header.sender = headerBytes[2];
    header.checksum = headerBytes[3];

    data.resize(header.length);
    return readBytes(listenAddress(), data);
}

void Zero2WI2C_pigpio::switchToControllerMode()
{
    log() << "Controller mode not available via pigpio\n";
    throw std::runtime_error("Controller mode not available via pigpio");
}

void Zero2WI2C_pigpio::switchToResponderMode(uint8_t address, protocols::MsgCallback cb)
{
    open();

    if (!initialized() || listening()) {
        log() << std::format("Not initialized ({}) or already in Responder mode. ({})\n", !initialized(), listening());
        return;
    }
    if (listening()) {
        if (address == listenAddress()) {
            log() << std::format("Already in Responder mode on address 0x{:02x}.\n", address);
            return;
        }
        stopListening();
    } else /*if (verbose())*/ {
        log() << std::format("Switching to Responder mode on address 0x{:02x}.\n", address);
    }
    listenAddress(address);
    callback(cb);

    startListening();
}
