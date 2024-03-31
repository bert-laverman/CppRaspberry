// Copright (c) 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47

#include <pico/pico.hpp>

using namespace nl::rakis::raspberrypi;


#if defined(HAVE_SPI)

void interfaces::SPI::numModules(unsigned numModules)
{
    numModules_ = numModules;
    if (device_ != nullptr)
    {
        device_->numModulesChanged(numModules);
    }
}

#endif