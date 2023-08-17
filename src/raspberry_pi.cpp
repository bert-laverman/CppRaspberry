// Copyricht (c) 2023 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47

#include <raspberry_pi.hpp>

using namespace nl::rakis::raspberrypi;

RaspberryPi *RaspberryPi::instance_ = nullptr;

#if defined(TARGET_PICO)

#include <pico/pico.hpp>

RaspberryPi *RaspberryPi::instance(bool verbose) {
    if (instance_ == nullptr) {
        instance_ = new PICO(verbose);
    }
    return instance_;
}


#elif defined(TARGET_ZERO2W)

#include <zero2w/zero2w.hpp>

RaspberryPi *RaspberryPi::instance(bool verbose) {
    if (instance_ == nullptr) {
        instance_ = new Zero2W(verbose);
    }
    return instance_;
}

#else

#error "No target defined"

#endif
