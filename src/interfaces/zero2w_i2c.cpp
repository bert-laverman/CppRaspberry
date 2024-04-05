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