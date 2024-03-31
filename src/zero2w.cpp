// Copyright 2024 by Bert Laverman, All rights reserved.
// Created: 2021-09-11 14:59:47

#include <raspberry_pi.hpp>

#include <zero2w/zero2w.hpp>

using namespace nl::rakis::raspberrypi;

RaspberryPi *RaspberryPi::instance(bool verbose) {
    if (instance_ == nullptr) {
        instance_ = new Zero2W(verbose);
    }
    return instance_;
}
