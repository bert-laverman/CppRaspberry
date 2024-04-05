// Copyricht (c) 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47

#if defined(HAVE_SPI)

#include <interfaces/spi.hpp>

using namespace nl::rakis::raspberrypi;

void interfaces::SPI::numModules(unsigned numModules)
{
    numModules_ = numModules;
    if (device_ != nullptr)
    {
        device_->numModulesChanged(numModules);
    }
}

#endif