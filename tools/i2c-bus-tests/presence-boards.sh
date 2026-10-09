#!/usr/bin/env bash
#
# The board side of the presence test, to be used together with a controller that you start by hand:
#
#   on the controller host (a terminal there):    ~/test-i2c 80 0x61 0x62
#   here, at the same moment:                     SER_A=... SER_B=... ./presence-boards.sh
#
# After the controller has started, this restarts board A, puts board B in BOOTSEL mode (where it does not run), and starts
# it again. It says what it does and when. In the output of the controller you should see (the times are about the same):
#
#   * both boards "appeared" when the controller started, without having been restarted,
#   * board A "appeared" again after it was restarted,
#   * board B "is gone" about 10 seconds after it was put in BOOTSEL mode, and "appeared" again when it was started.
#
# Settings (environment): BUILD_HOST (where the Picos are plugged in, default pi5), SER_A, SER_B (USB serial numbers of the
# running Picos, see cppr-deploy --list-picos), and START_AFTER, seconds to wait before the first action (default 15).

set -u
BUILD_HOST=${BUILD_HOST:-pi5}
SER_A=${SER_A:?set SER_A to the USB serial number of the first Pico (see cppr-deploy --list-picos)}
SER_B=${SER_B:?set SER_B to the USB serial number of the second Pico}
START_AFTER=${START_AFTER:-15}
now() { date +%H:%M:%S; }

echo "$(now) Start the controller now. First action in $START_AFTER seconds."
sleep "$START_AFTER"

echo "$(now) Board A restarts"
ssh "$BUILD_HOST" bash -s -- "$SER_A" <<'REMOTE'
for d in /sys/bus/usb/devices/*; do
  [ "$(cat $d/idVendor 2>/dev/null)" = 2e8a ] && [ "$(cat $d/serial)" = "$1" ] && ~/bin/picotool reboot -f --bus "$(cat $d/busnum)" --address "$(cat $d/devnum)" >/dev/null 2>&1
done
REMOTE
sleep 22

echo "$(now) Board B goes to BOOTSEL mode"
BUS_B=$(ssh "$BUILD_HOST" bash -s -- "$SER_B" <<'REMOTE'
for d in /sys/bus/usb/devices/*; do
  [ "$(cat $d/idVendor 2>/dev/null)" = 2e8a ] && [ "$(cat $d/serial)" = "$1" ] || continue
  ~/bin/picotool reboot -u -f --bus "$(cat $d/busnum)" --address "$(cat $d/devnum)" >/dev/null 2>&1
  cat $d/busnum
done
REMOTE
)
sleep 22

echo "$(now) Board B starts again"
ssh "$BUILD_HOST" bash -s -- "$BUS_B" <<'REMOTE'
for d in /sys/bus/usb/devices/*; do
  [ "$(cat $d/idVendor 2>/dev/null)" = 2e8a ] && [ "$(cat $d/idProduct)" = 0003 ] && [ "$(cat $d/busnum)" = "$1" ] || continue
  ~/bin/picotool reboot --bus "$1" --address "$(cat $d/devnum)" >/dev/null 2>&1
done
REMOTE
echo "$(now) Done. Wait for the controller to finish, and look at its output."
