// Copyright 2023 by Bert Laverman, All rights reserved.

#include <raspberry_pi.hpp>


#include <string>
#include <vector>
#include <iostream>

using namespace nl::rakis::raspberry;

inline char hex(int value)
{
    return char(value < 10 ? '0' + value : 'a' + value - 10);
}

int main(int argc, const char** argv )
{
    RaspberryPi& berry(*RaspberryPi::instance());

    for (unsigned i = 0x20 ; i < 0x30 ; i++) {
        std::cerr << "Device " << hex(i) << ": ";
        if (berry.i2c().write(i, std::span<uint8_t>())) {
            std::cerr << "Found" << std::endl;
        } else {
            std::cerr << "Not found" << std::endl;
        }
    }

}