<!--
  -- Copyright (c) 2024 by Bert Laverman. All Rights Reserved.
  --
  -- Licensed under the Apache License, Version 2.0 (the "License");
  -- you may not use this file except in compliance with the License.
  -- You may obtain a copy of the License at
  --
  --    http://www.apache.org/licenses/LICENSE-2.0
  --
  -- Unless required by applicable law or agreed to in writing, software
  -- distributed under the License is distributed on an "AS IS" BASIS,
  -- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  -- See the License for the specific language governing permissions and
  -- limitations under the License.
  -->

# CppRaspberry - A library for building a cockpit using the Raspberry Pi

This library provides C++ classes for building (parts of) a cockpit, using Raspberry Pis to control the components.

## Setup of the library

I chose `cmake` as the build system. I am building everything using WSL2 on Windows 11, with the Pico SDK for targeting the Raspberry Pi Pico, and a local cross-compiling GCC setup for the "larger" Pis.

### How to include this library in your project

See [the templates](project-templates/) in this repository. If you have an environment variable named `CPP_RASPBERRY_PATH` that points to the root of this repository, then the template will pick it up. If the variable is not found, you can default it to "`../CppRaspberry`".

### Selecting a target platform

In the `CMakeLists.txt` file in your project, you can choose the target platform using:

* `TARGET_PICO`
* `TARGET_ZERO2W` (this will probably be fine for several other models, but I havemnn't tested that)

### Selecting options

If you want, you can reduce the size of your application by selecting what interfaces and devices to include. This list will grow.

* `HAVE_I2C` - Include support for I2C.
* `HAVE_SPI` - Include support for SPI.
* `HAVE_PWM` - Include support for PWM.
* `HAVE_MAX7219` - Include support for the MAX7219 LED driver.

## Structure of the library

In the `src` and `include` directories you will find only those sources that are common to all configurations.

The `pico` and `zero2w` directories contain specific implementations for those two platforms. Within these, you can find sub-modules for I2C ans SPI support.

Other modules are for devices and components. A device is generally something like a MAX7219 to drive 7-segment led display, a TLC59711 to drive LEDs, an MCP23S17 to provide expansion for buttons and switches, an so on. Components are the things that you actually want to use, without having to worry about how it is connected to your Pi.

We use the following terms you may see in directory names:

* Interfaces

  An interface provides connectivity to devices. In our case, we use I2C, SPI, GPIO, and PWM. (a GPIO pin in PWM mode).

* Devices

  A device that we can talk to, typically using an interface. For example, a MAX7219 7-Segment LED driver.

* Component

  A component is something that we want to use. In rare cases it will also be a device, but often it is what we use through a device. Examples are LEDs, buttons, switches, and so on. Components are connected through devices or, in special cases, directly to an interface such as a GPIO pin.

### Local, remote, or virtual components

If a component is local, then it is connected to the Pi the software is running on. If remote, we need to send I2C messages to another Pi. A virtual component is one that is part of a group that has a single device or component, but we want to address eparately. For example, if you have a set of Max7219 devices on the same SPI bus, you cannot really address them individually. You need to address the group as a whole.

## Pi-to-Pi communication

There are 4 major ways in which you can connect several Pis together:

1. Using a serial connection. (UART) This gives you a full-duplex connection, but normally just for a single Pi to a single Pi. I want several Pis to be able to talk to each other, so I decided not to use this.
2. Using SPI. This is a half-duplex connection with one side in control, in the sense that the controller needs to explicitly switch directions, even though it uses separate pins for each data path. A Chip Select line is used to tell the component on the other side that you are going to exchange data. You can connect multiple Pis to the same connection, but each needs a separate Chip-Select line, so this is not really scalable.
3. I2C provides an actual [Bus](https://en.wikipedia.org/wiki/Bus_(computing)) in the sense that you can have multiple devices on the same medium, where there is no dedicated controller. This means we can connect lots of Picos to the same bus, and everyone can initiate a transfer. Individual devices use an address to identify themselves, so messages can be targeted at a specific device. Alternatively you can send messages to address 0, which is called a "General Call" and can be received by all. The I2C protocol also specifies a bidirectional message option, allowing for request-reply messages. Most I2C enabled devices use this to allow for querying of values.
4. The last option is also a Bus, namely a WiFi connection, but that requires all Pis to have the WiFi network configured.

I chose the I2C Bus, because I want the Picos to be able to work with the same firmware on all devices. This means that they will initially need to discover what their address is going to be, by using an equivalent approach as DHCP:

1. The Pico starts by only listening for General Call messages, which don't require you to specify an address.
2. A "Bus Controller", in my case a Zero 2W, will regularly send a General Call message (termed a "Hello" message) with a 64-bit device Id set to zero. This identifies it as the Bus Controller, and the "sender" field in the messages identifies its own address on the bus.
3. The Picos will pick up this GC Hello and use the "sender" field to send a targeted Hello back with their own device Id and the "sender" field set to zero.
4. The Bus Controller will respond to any targeted Hello message with a sender set to zero using a "SetAddress" GC message, which contains the Pico's device Id and address.
5. The Pico picks up "SetAddress" GC messages. If the device Id matches, it configures its I2C interface to switch to using that address.

### Non-Pico Raspberry Pis and I2C "Slave" mode

Strangely enough, the smallest Pis are the most capable with respect to I2C. They provide hardware support for so-called "Slave mode", which will be able to generate IRQs when a message comes in for the configured address. The larger Pis generally lack this support, as they are supposed to be "Masters". The PiGPIO library however does allow you to use the BCM chip to setup a "Slave" mode, be it that this uses the pins also used for SPI. Also, this particular interface _only_ does "Slave" mode, so you can't use it to talk to other Pis on the same bus.

As a consequence, my first implementation uses two I2C busses, one for "Normal Pi to Picos", and the other bus for the reverse.
