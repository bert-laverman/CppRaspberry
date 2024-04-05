// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2024-05-04


#include <zero2w/interfaces/zero2w_i2c.hpp>


using nl::rakis::raspberrypi::protocols::MsgHeader;
using namespace nl::rakis::raspberrypi::interfaces;


Zero2WI2C::Zero2WI2C(const char *interface)
    : I2C(verbose)
    , interface_{interface}
    , address_{address}
{
    if (verbose) {
        log() << "Zero2WI2C::Zero2WI2C(" << interface << ", " << int(address) << ")\n";
    }
    open();
}


Zero2WI2C::~Zero2WI2C()
{
    if (verbose()) {
        log() << "Zero2WI2C::~Zero2WI2C()\n";
    }
    close();
}

void Zero2WI2C::open()
{
    if (fd_ >= 0) {
        return true;
    }

    if (verbose()) {
        log() << "Opening '" << interface_ << "'\n";
    }
    fd_ = ::open(interface_.c_str(), O_RDWR);
    if (fd_ < 0) {
        if (verbose()) {
            log() << "Failed to open '" << interface_ << "'. Errno=" << errno << ".\n";
        }
        return false;
    }
    return true;
}

void Zero2WI2C::close() {
    if (fd_ >= 0) {
        if (verbose()) {
            log() << "Closing '" << interface_ << "'\n";
        }
        ::close(fd_);
        fd_ = -1;
    }
}


void Zero2WI2C::switchToControllerMode()
{
    if (verbose()) {
        log() << "Switching to Controller mode\n";
    }
    if (ioctl(fd_, I2C_SLAVE, address) < 0) {
        if (verbose()) {
            log() << "Failed to switch to Controller mode. Errno=" << errno << ".\n";
        }
        return false;
    }
    return true;
}

void Zero2WI2C::switchToResponderMode(uint8_t address, MsgCallback cb)
{
    if (verbose()) {
        log() << "Switching to Responder mode\n";
    }
    if (ioctl(fd_, I2C_SLAVE, address) < 0) {
        if (verbose()) {
            log() << "Failed to switch to Responder mode. Errno=" << errno << ".\n";
        }
        return false;
    }
    return true;
}