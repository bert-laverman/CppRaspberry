# GitHub Copilot Instructions

C++20 library providing a uniform API to control Raspberry Pi Pico and Raspberry Pi Zero 2W boards, including GPIO, I2C, SPI, device controllers (MAX7219, LCD, OLED, etc.), and small components (buttons, LEDs).

## Build System

The library is consumed via CMake. Projects include `CppRaspberry.cmake` and call `add_cpp_raspberry_app(${APP_NAME})`.

**Required CMake variables:**
- `APP_NAME` — must be defined before including `CppRaspberry.cmake`
- Exactly one platform target: `TARGET_PICO` or `TARGET_ZERO2W`

**Optional feature flags** (set to `on` to include):
```cmake
set(HAVE_I2C on)
set(HAVE_SPI on)
set(HAVE_MAX7219 on)   # also requires HAVE_SPI
set(HAVE_LCD2X16 on)   # also requires HAVE_I2C
```

Devices and components are loaded by iterating `devices/devices.txt` and `components/components.txt`. To enable a new device or component, add its directory name to the appropriate `.txt` file (lines starting with `#` are comments).

See `project-templates/pico/CMakeLists.txt` and `project-templates/zero2w/CMakeLists.txt` for complete project setup examples.

## Architecture

The library is structured in four layers:

```
platforms/          Platform-specific implementations (GPIO, I2C, SPI drivers)
  pico/             Raspberry Pi Pico (RP2040) — uses Pico SDK
  zero2w/           Raspberry Pi Zero 2W — uses /dev/i2c, /dev/spidev, or pigpiod

interfaces/         Platform-agnostic interface contracts (I2C, SPI)
  i2c/              Common I2C bus definitions and protocol driver
  spi/              Common SPI bus definitions and device base classes

devices/            Hardware device controllers (one subdirectory per device family)
components/         High-level reusable components (button, led, 7segment)

include/            Common base classes (GPIO, util/, protocols/)
src/                (currently empty; platform sources live under platforms/)
```

**Key architectural pattern — Local/Remote split:**  
Each device has abstract base, `Local*` (hardware on same board via SPI/I2C), and optionally `Remote*` (proxied over I2C to another board). Example: `MAX7219`, `LocalMAX7219`, `RemoteMAX7219`.

**Zero 2W I2C implementations:**  
Two backends exist: `I2cdevI2C` (uses `/dev/i2c-X`) and `PigpiodI2C` (uses the pigpiod daemon). Similarly for SPI: `SpidevSPI` and `PigpiodSPI`.

## Code Style

**Naming:**
- Types: `PascalCase`
- Files and directories: `lowercase-with-hyphens` (snake_case for header filenames matching class name)
- Fields: `camelCase` with trailing underscore — `verbose_`, `writeImmediately_`
- Getters/setters share the field name without the underscore: `bool verbose() const`, `void verbose(bool v)`

**C++ conventions:**
- C++20; use `constexpr`, `const`, and `noexcept` wherever applicable
- Prefer CRTP over virtual dispatch — delegate to derived via `static_cast<Derived*>(this)->doX()`
- 4-space indentation
- `#pragma once` at the top of every header, before the license comment

**Doxygen comments:**
- Every class and non-trivial method gets a `/** ... */` comment
- Do **not** use `@brief` — write the description directly
- Use `@param` and `@return` tags
- Default/deleted special members do **not** get a comment

**License header** — every new file must start with `#pragma once` (headers) then:
```cpp
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
```

## Adding a New Device

1. Create `devices/<name>/` with `<name>.cmake`, `include/devices/<name>.hpp`, and optionally `src/devices/<name>.cpp`.
2. The `.cmake` file appends to `CPP_RASPBERRY_INCLUDES`, `CPP_RASPBERRY_SOURCES`, and `CPP_RASPBERRY_LIBS`.
3. Add `<name>` on a new line in `devices/devices.txt`.
4. If the device requires SPI or I2C, guard the header with `#if !defined(HAVE_SPI)` / `#if !defined(HAVE_I2C)` and `#error`.

## Adding a New Component

Same structure as devices but under `components/` and `components/components.txt`. Component headers go in `include/components/`.

## Namespace Layout

```
nl::rakis::raspberrypi           Top-level (raspberry-pi.hpp, RaspberryPi class)
nl::rakis::raspberrypi::interfaces   GPIO, I2C, SPI interface classes
nl::rakis::raspberrypi::devices      Device controllers
nl::rakis::raspberrypi::components   High-level components
nl::rakis::raspberrypi::protocols    Message/protocol driver base classes
nl::rakis::raspberrypi::util         NamedComponent, VerboseComponent, MessageQueue
```

Subdirectory structure under `include/` and `src/` mirrors the sub-namespace (e.g. `include/interfaces/gpio.hpp` → `nl::rakis::raspberrypi::interfaces::GPIO`).
