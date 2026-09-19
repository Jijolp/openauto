#!/usr/bin/env python3
"""CanBridge simulator: replays plausible W203 traffic on a (virtual) CAN bus.

Frames match car/can_map.json PLACEHOLDER ids (0x260 ignition, 0x261 speed,
0x262 lights, 0x266/0x267 steering-wheel buttons).

Usage:
    sudo pacman -S python-can            # one-time dependency
    sudo modprobe vcan                   # one-time per boot (human, needs sudo)
    sudo ip link add dev vcan0 type vcan # one-time per boot (human, needs sudo)
    sudo ip link set up vcan0            # one-time per boot (human, needs sudo)
    python3 car/sim.py                   # loop: ignition + speed ramp + buttons
    python3 car/sim.py --once            # single pass then exit
    python3 car/sim.py --interface can0  # real hardware later

Run from the OpenAuto directory so relative paths stay valid.
"""
import argparse
import sys
import time

try:
    import can
except ImportError:
    sys.exit("python-can is missing: sudo pacman -S python-can")

IGNITION_ID = 0x260
SPEED_ID = 0x261
NIGHT_ID = 0x262
BTN_A_ID = 0x266  # seek_next 0x08, seek_prev 0x04, toggle_play 0x02, voice 0x01
BTN_B_ID = 0x267  # ok_enter 0x01, back 0x02

# (frame id, byte0 value, label) — each press is followed by a release.
BUTTON_SEQUENCE = [
    (BTN_A_ID, 0x08, "seek_next NEXT"),
    (BTN_A_ID, 0x04, "seek_prev PREV"),
    (BTN_A_ID, 0x02, "toggle_play TOGGLE_PLAY"),
    (BTN_A_ID, 0x01, "voice MICROPHONE_1"),
    (BTN_B_ID, 0x01, "ok ENTER"),
    (BTN_B_ID, 0x02, "back BACK"),
]

PRESS_HOLD_S = 0.3


def send(bus, arb_id, byte0, label=""):
    msg = can.Message(arbitration_id=arb_id, data=[byte0] + [0] * 7,
                      is_extended_id=False)
    bus.send(msg)
    print(f"TX {arb_id:#05x} [{byte0:#04x}] {label}", flush=True)


def single_pass(bus):
    send(bus, IGNITION_ID, 0x01, "ignition ON")
    send(bus, NIGHT_ID, 0x04, "night ON")
    # GALA-style progressive speed: 0 -> 90 -> 0 km/h, 5 km/h steps.
    for speed in list(range(0, 95, 5)) + list(range(85, -1, -5)):
        send(bus, SPEED_ID, speed, f"speed {speed} km/h")
        time.sleep(0.1)
    for arb_id, value, label in BUTTON_SEQUENCE:
        send(bus, arb_id, value, f"PRESS {label}")
        time.sleep(PRESS_HOLD_S)
        send(bus, arb_id, 0x00, f"RELEASE {label}")
        time.sleep(0.4)


def main():
    parser = argparse.ArgumentParser(description="CanBridge vcan simulator")
    parser.add_argument("--interface", default="vcan0")
    parser.add_argument("--once", action="store_true",
                        help="single pass instead of looping")
    args = parser.parse_args()

    try:
        bus = can.interface.Bus(channel=args.interface, interface="socketcan")
    except OSError as exc:
        sys.exit(f"cannot open {args.interface}: {exc} "
                 f"(sudo ip link add dev {args.interface} type vcan && "
                 f"sudo ip link set up {args.interface})")

    print(f"simulator on {args.interface}, Ctrl-C to stop", flush=True)
    try:
        if args.once:
            single_pass(bus)
        else:
            while True:
                single_pass(bus)
                time.sleep(1.0)
    except KeyboardInterrupt:
        pass
    finally:
        bus.shutdown()


if __name__ == "__main__":
    main()
