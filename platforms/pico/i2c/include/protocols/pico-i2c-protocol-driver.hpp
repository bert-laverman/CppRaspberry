#pragma once
/*
 * Copyright (c) 2024 by Bert Laverman. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#if !defined(TARGET_PICO)
#error "The PicoI2CProtocolDriver runs only on the Raspberry Pi Pico"
#endif
#if !defined(HAVE_I2C)
#error "The PicoI2CProtocolDriver requires I2C support"
#endif

#include <pico/sync.h>


#include <util/pico-message-queue.hpp>
#include <interfaces/pico-i2c.hpp>
#include <protocols/messages.hpp>
#include <protocols/i2c-protocol-driver.hpp>


namespace nl::rakis::raspberrypi::protocols {

using PicoI2CProtocolDriver = I2CProtocolDriver<util::PicoMessageQueue>;

} // namespace nl::rakis::raspberrypi::protocols