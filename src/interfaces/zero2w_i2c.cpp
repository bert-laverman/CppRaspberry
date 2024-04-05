// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2024-05-04


extern "C" {
#include <i2c/smbus.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
}

#include <zero2w/interfaces/zero2w_i2c.hpp>


using nl::rakis::raspberrypi::protocols::MsgHeader;
using namespace nl::rakis::raspberrypi::interfaces;


Zero2WI2C::~Zero2WI2C()
{
    close();
}

void Zero2WI2C::open()
{
    if (fd_ >= 0) {
        return;
    }

    if (verbose()) {
        log() << "Opening '" << interface_ << "'\n";
    }
    fd_ = ::open(interface_.c_str(), O_RDWR);
    if (fd_ < 0) {
        if (verbose()) {
            log() << "Failed to open '" << interface_ << "'. Errno=" << errno << ".\n";
        }
    } else {
        initialized(true);
    }
}

void Zero2WI2C::close() {
    if (fd_ >= 0) {
        if (verbose()) {
            log() << "Closing '" << interface_ << "'\n";
        }
        ::close(fd_);
        fd_ = -1;
        initialized(false);
    }
}


void Zero2WI2C::switchToControllerMode()
{
    if (verbose()) {
        log() << "Switching to Controller mode\n";
    }
    reset();

    controller(true);
}

void Zero2WI2C::switchToResponderMode(uint8_t address, MsgCallback cb)
{
    if (verbose()) {
        log() << "Switching to Responder mode on address 0x" << hexHigh(address) << hexLow(address) << ".\n";
    }
    reset();

    if (ioctl(fd_, I2C_SLAVE, address) < 0) {
        if (verbose()) {
            log() << "Failed to switch to Responder mode. Errno=" << errno << ".\n";
        }
        close();

        return;
    }
    listenAddress(address);
    callback(cb);
    controller(false);
}

void Zero2WI2C::writeBytes(uint8_t address, std::span<uint8_t> data)
{
    open();

    struct i2c_msg msg{ address, 0, static_cast<__u16>(data.size()), data.data() };
    struct i2c_rdwr_ioctl_data msgs{ &msg, 1 };

    auto result = ioctl(fd_, I2C_RDWR, &msgs);
    if (result != 1) {
        log() << "Failed to write " << data.size() << " bytes to 0x" << hexHigh(address) << hexLow(address) << ". Errno=" << errno << ".\n";
    }
}
