// Copyright 2023 by Bert Laverman, All rights reserved.

#include <raspberry_pi.hpp>


#include <string>
#include <vector>
#include <array>
#include <iostream>

using namespace nl::rakis::raspberrypi;

inline char hex(int value)
{
    return char(value < 10 ? '0' + value : 'a' + value - 10);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv )
{
    RaspberryPi& berry(*RaspberryPi::instance(true));

    for (unsigned i = 0x20 ; i < 0x30 ; i++) {
        std::cerr << "Device 0x" << hex(i >> 4) << hex(i & 0x0f) << ": ";

        std:array<uint8_t, 16> cmd = { 0x00 };
        if (berry.i2c().write(i, cmd.data(), 1) {
            std::cerr << "Found" << std::endl;
        } else {
            std::cerr << "Not found" << std::endl;
        }
    }
    return 0;
}