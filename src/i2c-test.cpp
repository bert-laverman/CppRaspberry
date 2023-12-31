// Copyright 2023 by Bert Laverman, All rights reserved.

#include <raspberry_pi.hpp>


#include <string>
#include <vector>
#include <array>
#include <iostream>

#include <devices/lcd2x16.hpp>

using namespace nl::rakis::raspberrypi;

inline char hex(int value)
{
    return char(value < 10 ? '0' + value : 'a' + value - 10);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv )
{
    RaspberryPi& berry(*RaspberryPi::instance(false));

    devices::LCD2x16 lcd(berry.i2c());

    std::cerr << "Initialising display" << std::endl;
    lcd.initDisplay();

    std::cerr << "Clearing display" << std::endl;
    lcd.clear();
    lcd.light();

    std::cerr << "Printing first line" << std::endl;
    lcd.print(1, "Hello, world!");
    berry.sleepMs(5000);

    std::cerr << "Goodbye" << std::endl;
    lcd.clear();
    lcd.dark();

    return 0;
}