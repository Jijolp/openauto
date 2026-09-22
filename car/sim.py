#!/usr/bin/env python3
"""CanBridge simulator: replays plausible W203 traffic on a (virtual) CAN bus.

Frames match car/can_map.json PLACEHOLDER ids (0x260 ignition, 0x261 speed,
0x262 lights, 0x263 temp_ext, 0x264 rpm, 0x266/0x267 steering-wheel buttons).

Usage:
    sudo pacman -S python-can            # one-time dependency
    sudo modprobe vcan                   # one-time per boot (human, needs sudo)
    sudo ip link add dev vcan0 type vcan # one-time per boot (human, needs sudo)
    sudo ip link set up vcan0            # one-time per boot (human, needs sudo)
    python3 car/sim.py                   # loop: ignition + speed ramp + buttons
    python3 car/sim.py --once            # single pass then exit    python3 car/sim.py --race            # loop: Race Mode telemetry (speed 0->130->0 + gear-shifted RPM)
    python3 car/sim.py --race --once     # single race pass then exit
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
TEMP_ID = 0x263
RPM_ID = 0x264  # PLACEHOLDER: byte0 = rpm/10 (Race Mode v1, calibrate on capture)
BTN_A_ID = 0x266  # seek_next 0x08, seek_prev 0x04, toggle_play 0x02, voice 0x01
BTN_B_ID = 0x267  # ok_enter 0x01, back 0x02, vol_up 0x04, vol_down 0x08

# (frame id, byte0 value, label) — each press is followed by a release.
BUTTON_SEQUENCE = [
    (BTN_A_ID, 0x08, "seek_next NEXT"),
    (BTN_A_ID, 0x04, "seek_prev PREV"),
    (BTN_A_ID, 0x02, "toggle_play TOGGLE_PLAY"),
    (BTN_A_ID, 0x01, "voice MICROPHONE_1"),
    (BTN_B_ID, 0x01, "ok ENTER"),
    (BTN_B_ID, 0x02, "back BACK"),
    (BTN_B_ID, 0x04, "vol_up VOLUME_UP"),
    (BTN_B_ID, 0x08, "vol_down VOLUME_DOWN"),
]

PRESS_HOLD_S = 0.3


def send(bus, arb_id, byte0, label=""):
    msg = can.Message(arbitration_id=arb_id, data=[byte0] + [0] * 7,
                      is_extended_id=False)
    bus.send(msg)
    print(f"TX {arb_id:#05x} [{byte0:#04x}] {label}", flush=True)


def rpm_for_speed(speed_kmh):
    """Coherent fake gearbox: 5 gears, rpm climbs 1200->6500 inside each gear.

    Pure placeholder so the Race gauge + redline move plausibly with speed.
    Real W203 RPM frames will replace this after the Quadlock capture.
    """
    gear = min(5, 1 + int(speed_kmh) // 26)  # ~0-25:1, 26-51:2, ..., 104+:5
    lo = (gear - 1) * 26
    frac = (speed_kmh - lo) / 26.0 if gear < 5 else (speed_kmh - lo) / 30.0
    frac = max(0.0, min(1.0, frac))
    return int(1200 + frac * (6500 - 1200))


def send_speed_rpm(bus, speed):
    send(bus, SPEED_ID, int(speed) & 0xFF, f"speed {speed} km/h")
    rpm = rpm_for_speed(speed)
    send(bus, RPM_ID, (rpm // 10) & 0xFF, f"rpm {rpm} (byte0=rpm/10)")


def single_pass(bus):
    send(bus, IGNITION_ID, 0x01, "ignition ON")
    send(bus, NIGHT_ID, 0x04, "night ON")
    send(bus, TEMP_ID, 60, "temp 20°C (60-40)")
    # GALA-style progressive speed: 0 -> 90 -> 0 km/h, 5 km/h steps.
    for speed in list(range(0, 95, 5)) + list(range(85, -1, -5)):
        send_speed_rpm(bus, speed)
        time.sleep(0.1)
    for arb_id, value, label in BUTTON_SEQUENCE:
        send(bus, arb_id, value, f"PRESS {label}")
        time.sleep(PRESS_HOLD_S)
        send(bus, arb_id, 0x00, f"RELEASE {label}")
        time.sleep(0.4)


def race_pass(bus, step_delay=0.12):
    """Race Mode telemetry: 0 -> 130 -> 0 km/h with gear-shifted RPM."""
    send(bus, IGNITION_ID, 0x01, "ignition ON")
    speeds = list(range(0, 132, 2)) + list(range(128, -1, -2))
    for speed in speeds:
        send_speed_rpm(bus, speed)
        time.sleep(step_delay)


def main():
    parser = argparse.ArgumentParser(description="CanBridge vcan simulator")
    parser.add_argument("--interface", default="vcan0")
    parser.add_argument("--once", action="store_true",
                        help="single pass instead of looping")
    parser.add_argument("--race", action="store_true",
                        help="Race Mode telemetry loop (speed 0->130->0 + RPM)")
    args = parser.parse_args()

    try:
        bus = can.interface.Bus(channel=args.interface, interface="socketcan")
    except OSError as exc:
        sys.exit(f"cannot open {args.interface}: {exc} "
                 f"(sudo ip link add dev {args.interface} type vcan && "
                 f"sudo ip link set up {args.interface})")

    print(f"simulator on {args.interface}, Ctrl-C to stop", flush=True)
    try:
        if args.race:
            if args.once:
                race_pass(bus)
            else:
                while True:
                    race_pass(bus)
                    time.sleep(1.0)
        elif args.once:
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
