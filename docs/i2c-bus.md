<!--
  -- Copyright (c) 2026 by Bert Laverman. All Rights Reserved.
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

# The I2C bus between a Zero and its Picos

This describes how a Raspberry Pi Zero 2 W (the *bus controller*) and any number of Raspberry Pi Picos (the *devices*) talk to
each other over I2C, what has been tested, and what went wrong on the way. The code is in this repository, in
`Zero2WTestI2C` (the controller), and in `PicoTestI2C` (a device). Status: 9 October 2026.

## Topology

All devices share one bus: SDA and SCL, plus **two** ground wires. The controller is both master and slave on the same
two wires, because a device must be able to send, too:

| Role | Controller (Zero 2 W) | Device (Pico) |
|---|---|---|
| Master | `/dev/i2c-1` (GPIO 2 and 3), through `I2CDevI2C` | `i2c1` (GP14 and GP15), by `PicoI2C` |
| Slave | the BSC slave of the chip (GPIO 18 and 19, wired to the same wires), through `pigpiod` and `PigpiodBSCI2C`, at address `0x0a` | the same `i2c1`, `PicoI2C` switches between the roles |

The Zero has 1.8 kΩ pull-ups on GPIO 2 and 3, which serve the whole bus. `pigpiod` runs on the Zero, listening on localhost only.

## Messages

A message is a header of four bytes, followed by the payload (`MsgHeader`, in `interfaces/i2c.hpp`):

| Byte | Field |
|---|---|
| 0 | `command` (`Hello` 0x00, `SetAddress` 0x01, `Enumerate` 0x02, `InterfaceInfo` 0x03, `DeviceInfo` 0x04, `Log` 0x0f, `Led` 0x10, `Max7219` 0x11, `Button` 0x12) |
| 1 | `length` of the payload |
| 2 | `sender`: the address of the sender, `0x00` if it has none yet |
| 3 | `checksum`: XOR of the payload (the header is not covered) |

There is no magic number, so the receiver finds the start of a message by checking: a known command, a valid sender, a
length of at most 64, and a matching checksum. It drops one byte at a time until it finds a message that passes
(`PigpiodBSCI2C::processBytes`). A partial message is dropped when the rest has not arrived within 20 ms.

## Giving devices an address

Devices start without an address and only listen for General Calls (address `0x00`).

1. The controller says `Hello` as a General Call once per second (`I2CBusController::tick()`). A device that hears it notes
   the sender as the controller's address.
2. A device without an address answers with a `Hello` to the controller (`0x0a`), carrying its board id (the flash id of the
   Pico). It repeats this once per second until it has an address.
3. The controller picks an address: the same one as before for a board it knows, otherwise the first free one from `0x61`. It
   keeps the address reserved for that board, and sends it as a `SetAddress` General Call (board id and address).
4. The device that the board id belongs to takes over the address, and **confirms** with a `Hello` sent from its new address.
5. Until the confirmation arrives, the assignment is *pending*: the controller repeats the `SetAddress` every 100 ms, at most 5
   times, and then gives up.
6. A confirmed address is saved by the application (`onConfirmed`), so a board gets the same address after a restart of the
   controller.

`I2CBusController` (controller) and `I2CDeviceHandler` (device) implement this.

## Presence

The controller knows whether a board is *online*. A board announces itself: once it has an address, it answers every third
`Hello` of the controller (so about every 3 seconds) with a `Hello` of its own, sent from its own address
(`I2CDeviceHandler::announceEvery()`). Any message from a board counts as a sign of life. `I2CBusController` notes when it
last heard from each board, and `tick()` checks that:

* A board that is silent for `goneAfter()` (10 seconds by default) is *gone*; `onBoardGone()` is called once.
* A board that is heard again, or confirms an address, is *appeared*; `onBoardAppeared()` is called. A board that talks and is
  not yet known is added to the known boards first.

The application decides what to do with it. `Zero2WTestI2C` logs it and sends a freshly appeared display its settings again.

Tested on 9 October 2026 with two Picos and a controller that was already running (`tools/i2c-bus-tests/presence-boards.sh`
does the board side):

| Action | Result |
|---|---|
| Controller starts, both boards already running | both `appeared` within seconds, no restart needed |
| Board A restarted | `gone`, then address confirmed again and `appeared` |
| Board B put in BOOTSEL mode (it does not run) | `gone` |
| Board B started again | address confirmed again and `appeared` |

A restarted board has no address and sends `Hello` from `0x00`, so it only counts as online again when the controller has
assigned its old address and the board has confirmed it. Stopping `Zero2WTestI2C` with SIGINT or SIGTERM now ends its loop and closes the BSC slave properly (tested). Killing it
with `kill -9` in the middle of a transfer used to leave the BSC slave of the chip enabled and holding `SCL` low; `PigpiodBSCI2C`
now switches the BSC slave off before it enables it, which should clear that after a restart. That case has not been tested
since the change.

## What was measured

* With a single ground wire the Pico saw every START and STOP, but misread the address bits. With two, everything worked.
* Two masters that start within the same bit time: the RP2040 reports `ARB_LOST` correctly, and the winner's message arrives
  intact. The loser's message is lost, which is what the once-per-second repeat of the address request is for.
* Two messages directly behind each other (24 bytes in 2.5 ms) overflowed the 16 byte receive FIFO of the BSC slave when the
  controller polled it every 10 ms. It polls every 1 ms now.
* Two Picos that ask for an address at the same moment (a trigger line makes them do so) both end up with a different address,
  in every one of the rounds that were run.
* A lost `SetAddress` is repaired by sending it again; a board that never confirms is given up on after 5 attempts. Both were
  tested with a test build of `PicoTestI2C` that ignores `SetAddress` messages.

The programs and the script for these tests are in `tools/i2c-bus-tests`.

## Things that were broken, and may bite again

* A C++ class that declares `sendMessage()` hides the overloads of its base class. `I2CProtocolDriver` brings them back with a
  `using` declaration.
* `LocalMAX7219::doReset()` used to leave `writeImmediately` off. It now restores what it was.
* `PicoI2C` never set its `listening` flag. That made `listenAddress()` and `stopListening()` do nothing.
* `I2CState::load()` makes keys lower case; look them up in lower case (`boardid`).
* `rsync --delete` in `cppr-deploy` removes build directories that are not excluded: `build*` is excluded.
* A Zero on wifi can hang SSH sessions that have no terminal (and `scp`). Switching off wifi power saving
  (`wifi.powersave = 2` in `/etc/NetworkManager/conf.d/`) fixed it here.

## Limits

* `RemoteMAX7219` can show numbers and blank, not single segments (there is no message for it).
* The controller and the devices have to agree on the number of display modules; the controller does not ask.
* Only one bus, one controller.
* The protocol has no version, no magic number, and a weak checksum. See the notes in `tools/i2c-bus-tests`.

## Building and testing

The sources live side by side in `~/dev/CppRaspberry/` (the library, and the `Pico*` and `Zero2W*` projects). `tools/cppr-deploy`
builds a project on a Raspberry Pi 5 (GCC 14, the Pico SDK, `picotool`) and flashes a Pico plugged into it, or copies a Zero
program to the Zero. See `tools/README.md`.
