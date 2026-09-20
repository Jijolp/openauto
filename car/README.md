# car/ — CanBridge config, simulator and capture procedure

`CanBridge` (OpenAuto, `Projection/CanBridge.*`) reads socketcan frames and
translates them into Android Auto actions. **All vehicle knowledge lives in
`can_map.json`** — real W203 IDs are plugged in here later, C++ untouched.

Relevant env vars (all optional):

| var | default | meaning |
|---|---|---|
| `OPENAUTO_ENABLE_CAN` (cmake) | `OFF` | build CanBridge in (`-DOPENAUTO_ENABLE_CAN=ON`) |
| `OPENAUTO_CAN_IF` | `vcan0` | socketcan interface to listen on |
| `OPENAUTO_CAN_MAP` | `car/can_map.json` | mapping file (relative to CWD, i.e. `OpenAuto/` via `run-dev.sh`) |

## JSON schema (`can_map.json`)

```json
{
  "buttons": [
    {"name": "seek_next", "can_id": "0x266", "byte": 0,
     "mask": "0x08", "press": "0x08", "release": "0x00",
     "aa_button": "NEXT"}
  ],
  "ignition":   {"can_id": "0x260", "byte": 0, "mask": "0x01", "on": "0x01", "off": "0x00"},
  "speed":      {"can_id": "0x261", "byte": 0, "factor": 1.0},
  "night_mode": {"can_id": "0x262", "byte": 0, "mask": "0x04", "on": "0x04"},
  "temp_ext":   {"can_id": "0x263", "byte": 0, "factor": 1.0, "offset": -40}
}
```

- `buttons[]` — `can_id` (hex string or number), `byte` index in the 8-byte
  payload, `mask` applied before compare, `press`/`release` = masked values
  that emit AA PRESS / RELEASE. Edges only (held buttons don't repeat).
  `aa_button` must be a keycode from the table below, else the entry is
  skipped with a warning. All `can_id`/`mask`/`press`/`release` accept
  `"0x…"` strings or plain numbers.
- `ignition` / `speed` / `night_mode` / `temp_ext` — optional objects; absent = disabled.
  `ignition` OFF → screen-off overlay (OLED black, 1-line synergy); `night_mode`
  → theme-night re-tint; `temp_ext` → bandeau `21°` (placeholder `--°` on PC,
  formula `value* factor + offset`, e.g. `offset -40` for signed offset).
  `speed` = one byte × `factor`, logged when it moves by ≥ 1 km/h (GALA placeholder).
- Current IDs are **factices** (`0x260`–`0x267` plus `0x263` temp, one shared byte with bit
  masks). Shared-byte masks are safe thanks to edge detection.

AA keycodes available in this aasdk proto snapshot (`ButtonCodeEnum.proto`):

```
NONE MENU HOME BACK PHONE CALL_END UP DOWN LEFT RIGHT ENTER
MICROPHONE_1 MICROPHONE_2 TOGGLE_PLAY NEXT PREV PLAY PAUSE SCROLL_WHEEL
VOLUME_UP VOLUME_DOWN
```

`VOLUME_UP/DOWN` were added with the InputSource work (Android values
24/25): they flow through the new channel as `KEYCODE_VOLUME_UP/DOWN`
(the legacy enum carries them too, harmlessly). There is still no
`PLAY_PAUSE` short name — use `TOGGLE_PLAY` (= `MEDIA_PLAY_PAUSE`).

Binding: at startup the factory unions the map's `aa_button` codes into the
declared discovery keycodes (legacy `supported_keycodes` + inputsource
`keycodes_supported` via the legacy→Android table), so the phone's
`BindingRequest`/`KeyBindingRequest` can cover them
(logs: `[InputService] binding request, scan codes count: N` and
`[InputSourceService] key binding request, keycodes count: M`). Buttons also stay subject to `openauto.ini`
`Input.*Button` checkboxes for the keyboard path.

## Manual test (no phone needed for parsing)

```sh
# one-time per boot (needs sudo — ask a human, the agent must not sudo):
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

# terminal 1: build with CAN and run (from OpenAuto/):
export AASDK_PATH=$HOME/dev/aa-headunit/aasdk
cmake -B build -DCMAKE_BUILD_TYPE=Release -DOPENAUTO_ENABLE_CAN=ON
cmake --build build -j$(nproc)
./run-dev.sh            # wait for the idle screen; CanBridge retries until vcan0 exists

# terminal 2: single frames (can-utils):
cansend vcan0 266#08    # PRESS seek_next  -> [CanBridge] button seek_next (87) PRESS
cansend vcan0 266#00    # RELEASE          -> [CanBridge] button seek_next (87) RELEASE
cansend vcan0 260#01    # ignition ON → wake
cansend vcan0 260#00    # ignition OFF → screen off (overlay noir)
cansend vcan0 261#32    # speed 50 km/h stub (0x32 = 50)
cansend vcan0 262#04    # night ON → theme-night
cansend vcan0 263#3C    # temp 20°C (0x3C=60, 60-40)
cansend vcan0 263#28    # temp 0°C  (0x28=40)

# or the full loop:
sudo pacman -S python-can   # one-time
python3 car/sim.py --once
```

Expected without phone: parse/info logs for every frame. Button injection
without an open input channel fails gracefully (`[SensorService] channel
error…` via the existing `onChannelError` path — note the historic copy-paste
in that log prefix, upstream). With phone: `BindingRequest scan codes
count > 0` and `ButtonEvent`s on the input channel (protocol-level success);
visible UI reaction is a BONUS gated by the §16 version test / InputSource.

## Real capture (W203 CAN-B, Quadlock) — procedure to come

1. Wire a CAN transceiver (e.g. MCP2515 + TJA1050, 83.3 kbit/s CAN-B) to the
   Quadlock CAN-H/CAN-L behind the Audio 20, interface up as `can0`
   (`ip link set can0 type can bitrate 83333`, triple-check H/L polarity).
2. Engine off, contact on: `candump -L can0 > contact_on.log`; press each
   steering-wheel key 3× (1 s holds); toggle lights; `candump` during a
   short drive for GALA speed frames (needs a driver, obviously).
3. Diff idle vs action logs, isolate repeating IDs → fill `can_map.json`
   (`can_id`, `byte`, `mask`, `press`, `release`), keep `aa_button` names.
4. Validate on bench: replay with `canplayer` or `cansend`, check CanBridge
   logs, then in-car test.
