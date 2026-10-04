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

# I2C bus tests

Small programs and a script to find out what happens on the I2C bus between a Raspberry Pi Zero 2 W (the bus controller)
and one or more Picos (the devices). They are meant for finding out *why* something does not work, so they talk to the
hardware directly and do not use the library, except for the last test.

The setup they were written for:

| Signal | Zero 2 W | Pico |
|---|---|---|
| SDA | GPIO 2 (pin 3), and GPIO 18 (pin 12) for the BSC slave | GP14 (pin 19) |
| SCL | GPIO 3 (pin 5), and GPIO 19 (pin 35) for the BSC slave | GP15 (pin 20) |
| Ground | pin 6 **and** pin 39 | GND, one wire per Pico. **Use two ground wires.** |
| Trigger | GPIO 17 (pin 11) | GP2 (pin 4), on every Pico |

The Zero's pull-ups on GPIO 2/3 (1.8 kΩ) serve the whole bus. All Picos have their own USB connection (for power, flashing
and serial output) and share only the three bus wires and the trigger line.

> A single ground wire was not enough here: the Pico saw the start and stop of every transfer, but misread the address
> bits, so nothing was ever acknowledged. With two ground wires everything worked at once.

## The tests

Each directory is a stand-alone Pico SDK project (`PICO_SDK_PATH` must be set). Build one with, for example:

```bash
cmake -S pinwatch -B pinwatch/build -G Ninja && cmake --build pinwatch/build
```

and load `build/<name>.uf2` with `picotool`. All of them print on the USB serial port.

### `pinwatch`

Reports the levels of GP14 (SDA) and GP15 (SCL), and prints a line when one changes and once per second. The Pico does not
enable any pull-up, so a high level can only come from the Zero. Use it to check the wiring: drive a line low on the Zero
(`pinctrl set 2 op dl`, and `pinctrl set 2 a0` to give it back) and watch it go low on the Pico.

### `slavetest`

A minimal I2C slave at address `0x61` on `i2c1`, written directly with the SDK, no library. Counts the bytes it receives,
and the START and STOP conditions it sees, and prints the interrupt bits that were set. If `i2cdetect -y -q 1` on the
Zero does not show `61` with this running, the problem is not in the library.

### `collision`

Wait for a rising edge on GP2 (the shared trigger line), then send a 12 byte `Hello` message to `0x0a` as an I2C master.
The registers are driven directly, because the SDK clears `TX_ABRT_SOURCE` when a write fails, which hides an `ARB_LOST`.
Every Pico sends its own board id, so the messages are the same up to the checksum byte and differ after that.

Per trigger it prints, for example `TRIAL 6: ARB_LOST abrt=0x4001000 time=392 us (delay 0 us)`:

| Outcome | Meaning |
|---|---|
| `SENT` | All 12 bytes were acknowledged. |
| `ARB_LOST` | Another master won the arbitration. In `abrt`, bit 12 is `ARB_LOST` and bits 31:23 count the bytes that were thrown away: `0x4001000` is 8 of 12, so the loss was in the 4th data byte, the checksum. |
| `NOACK_ADDRESS` | The controller did not acknowledge `0x0a`. |
| `NOACK_DATA` | A data byte was not acknowledged. |

Build with `-DDELAY_US=600` to make a Pico wait that long after the trigger. If the other Pico is halfway through its
message, this one finds the bus busy and sends right behind it. That produces two complete messages back to back, 24 bytes
in about 2.5 ms, which is what overflowed the 16 byte BSC receive FIFO of the controller before it was polled every 1 ms.

Run `test-i2c` on the controller (`Zero2WTestI2C`) to see what arrives there: it prints the raw bytes of every chunk
(`Received 12 bytes, now 12 in buffer: 00 08 00 6a …`) and every byte it throws away (`Dropping byte …`).

Pulse the trigger on the Zero with:

```bash
pinctrl set 17 op dh; sleep 0.01; pinctrl set 17 op dl; pinctrl set 17 ip
```

### `addr-run.sh`

Runs the whole address assignment (hello, request, `SetAddress`) with two Picos that ask for an address *at the same moment*.
It uses the **trigger build of `PicoTestI2C`** (`cmake -DTEST_TRIGGER=ON`), which holds back its first address request
until GP2 goes high and then sends it immediately. Per round:

1. restart the controller (`~/test-i2c`) on the controller host and remove its saved addresses (`~/i2c-state.ini`), so it
   has none yet,
2. reboot both Picos with `picotool reboot -f` and capture their serial output,
3. wait for them to hear the controller's `Hello`, and pulse the trigger,
4. collect the logs from the Picos and the controller, and print a summary.

```bash
SER_A=E6614104031A8938 SER_B=E6614C311B461728 ./addr-run.sh 8
```

`SER_A` and `SER_B` are the USB serial numbers of the *running* Picos (the flash id, see `cppr-deploy --list-picos`). The
other settings are `BUILD_HOST` (where the Picos are plugged in, default `pi5`), `CONTROLLER_HOST` (default `berry-1`),
`TRIGGER_GPIO` (default `17`) and `OUT` (where the logs go). The build host needs `picotool` in `~/bin`, and the controller
host needs `~/test-i2c` and `pinctrl`.

## What this showed (4 October 2026)

* With two ground wires, a Zero 2 W master and a Pico slave on the bus work reliably, in both directions: the Zero's master
  on `/dev/i2c-1` talks to the Pico, and the Pico (switching from slave to master for each message) talks to the Zero's
  `pigpiod` BSC slave on `0x0a`.
* Two masters that start within the same bit time: the RP2040 detects the arbitration loss correctly (`ARB_LOST`), and the
  winner's message arrives intact. The loser gets no second chance by itself, which is what the address request's
  once-per-second retry is for.
* Two messages directly behind each other arrive as one chunk of 24 bytes or as two of 12. Polling the BSC FIFO every 10 ms
  lost bytes (the FIFO holds 16); polling every 1 ms did not.
* Eight rounds in which two Picos asked for an address at the same moment all ended with two different addresses (`0x61`
  and `0x62`), and in each of them both Picos had to send their request only once.

  An earlier version needed a second request from both Picos in every round. The cause was not on the Pico, but in the
  controller: `test-i2c` only handled incoming messages once per second, so its `SetAddress` came just after the Pico
  repeated its request. Handling incoming messages every 10 ms fixed that. Switching `PicoI2C` between responder and master
  mode without resetting the whole I2C block (instead of a full reset after every write) also helps, but is not what made
  the difference.
* The address assignment is confirmed: a Pico that has taken over its new address sends a `Hello` from that address,
  and the controller keeps an address as *pending* until it sees that. A pending address is sent again every 100 ms, at most
  5 times. That path has not been exercised by these tests, because no assignment failed.
