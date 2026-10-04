#!/usr/bin/env bash
#
# Address-request collision test. Each round:
#   - restart the controller (test-i2c) on $CONTROLLER_HOST, and remove its saved addresses (~/i2c-state.ini), so it
#     has none yet
#   - reboot both Picos (they run the trigger build of PicoTestI2C) and capture their serial output
#   - wait until they have heard the controller, then pulse GPIO 17 on $CONTROLLER_HOST: both ask for an address at once
#   - collect the logs
#
#   SER_A=... SER_B=... addr-run.sh [rounds]
#
# Settings (environment): BUILD_HOST, CONTROLLER_HOST, TRIGGER_GPIO, SER_A, SER_B, OUT

set -u
ROUNDS=${1:-5}
BUILD_HOST=${BUILD_HOST:-pi5}                 # where the Picos are plugged in
CONTROLLER_HOST=${CONTROLLER_HOST:-berry-1}   # the Zero that runs the controller, and drives the trigger line
TRIGGER_GPIO=${TRIGGER_GPIO:-17}              # GPIO on the controller host that is wired to GP2 of every Pico
OUT="${OUT:-$(pwd)/addr-run-out}"
rm -rf "$OUT"; mkdir -p "$OUT"
SER_A=${SER_A:?set SER_A to the USB serial number of the first Pico (see cppr-deploy --list-picos)}
SER_B=${SER_B:?set SER_B to the USB serial number of the second Pico}
SSH="timeout 40 ssh -o BatchMode=yes -o ConnectTimeout=10"

find_picos='find_picos() { for d in /sys/bus/usb/devices/*; do [ "$(cat $d/idVendor 2>/dev/null)" = 2e8a ] || continue; echo "$(cat $d/busnum) $(cat $d/devnum) $(ls $d/*/tty 2>/dev/null | head -1) $(cat $d/serial)"; done; }'

for r in $(seq 1 "$ROUNDS"); do
    echo "== Round $r"

    # fresh controller
    $SSH "$CONTROLLER_HOST" 'pkill -x test-i2c; rm -f ~/i2c-state.ini; sleep 1; nohup ~/test-i2c 120 > ~/controller.log 2>&1 < /dev/null &' >/dev/null 2>&1
    sleep 2

    # reboot both Picos, wait for their serial ports, start capturing
    $SSH "$BUILD_HOST" bash -s -- "$SER_A" "$SER_B" "$r" <<REMOTE
$find_picos
for ser in "\$1" "\$2"; do
    read -r bus addr _ _ <<<"\$(find_picos | grep \$ser)"
    ~/bin/picotool reboot -f --bus "\$bus" --address "\$addr" >/dev/null 2>&1
done
sleep 2
for i in \$(seq 1 40); do [ "\$(find_picos | awk 'NF==4' | wc -l)" = 2 ] && break; sleep 0.5; done
ttyA=\$(find_picos | awk -v s="\$1" '\$4==s{print \$3}'); ttyB=\$(find_picos | awk -v s="\$2" '\$4==s{print \$3}')
rm -f ~/pico-A-\$3.log ~/pico-B-\$3.log
(nohup timeout 45 cat /dev/\$ttyA > ~/pico-A-\$3.log 2>&1 &)
(nohup timeout 45 cat /dev/\$ttyB > ~/pico-B-\$3.log 2>&1 &)
REMOTE

    sleep 10        # the Picos wait 5 s after power up, and then need to hear a Hello

    # the pulse
    $SSH "$CONTROLLER_HOST" "pinctrl set $TRIGGER_GPIO op dh; sleep 0.01; pinctrl set $TRIGGER_GPIO op dl; pinctrl set $TRIGGER_GPIO ip" >/dev/null 2>&1

    sleep 12        # retries (once per second), assignment

    # collect
    # the wifi link to the controller is not always quick, so try a few times
    for try in 1 2 3; do
        $SSH "$CONTROLLER_HOST" 'pkill -x test-i2c; sleep 1; cat ~/controller.log' > "$OUT/controller-$r.log" 2>/dev/null
        [ -s "$OUT/controller-$r.log" ] && break
    done
    for f in A B; do
        $SSH "$BUILD_HOST" "tr -d '\r' < ~/pico-$f-$r.log | grep -v '^\$'" > "$OUT/pico-$f-$r.log" 2>/dev/null
    done

    echo "   A: $(grep -E 'Triggered|Now listening|sendMessage' "$OUT/pico-A-$r.log" | sed 's/\[.*payload.*\]/(msg)/' | tr '\n' '|')"
    echo "   B: $(grep -E 'Triggered|Now listening|sendMessage' "$OUT/pico-B-$r.log" | sed 's/\[.*payload.*\]/(msg)/' | tr '\n' '|')"
    echo "   controller: $(grep -E "give this board|Dropping|Discarding" "$OUT/controller-$r.log" | tr '\n' '|')"
done
echo "Logs in $OUT"
