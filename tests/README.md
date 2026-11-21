# Host regression tests

These tests are maintained checks for positioning, MQTT/Home Assistant discovery,
remote mapping, base serial migration, RF transmission and decoding, and OTA
confirmation. They execute application code on the development computer with fake
clocks, radio/network interfaces, an in-memory filesystem or a small DOM harness.
They do not flash an ESP, contact an MQTT broker or move a shutter.

## Requirements

- Python 3.9 or newer.
- Node.js 18 or newer for the WebUI scripts and the base serial input check in
  `test_serial_migration.py`. These checks use only built-in modules.
- A C++17 compiler available as `c++`. The RF tests additionally require
  AddressSanitizer and UndefinedBehaviorSanitizer support, as provided by Clang
  or a suitably configured GCC installation.
- ArduinoJson 7 headers from the project's installed PlatformIO dependencies.
  The tests look under `.pio/libdeps/*/ArduinoJson/src`. Alternatively set
  `ARDUINOJSON_DIR` to an existing ArduinoJson `src` directory. Nothing is
  downloaded by the test runner.

## Run the suite

From the repository root:

```sh
python3 tests/run_tests.py
```

The runner is independent of the caller's current directory and stops on failure
with a nonzero exit code. Compilation outputs are created in temporary directories
and removed afterwards. Python bytecode caches are ignored locally in `tests/`.
Individual scripts can also be run with `python3 tests/test_….py` or
`node tests/test_….cjs`.

## Coverage

| Test | Purpose |
| --- | --- |
| `test_position_model.cpp` | Direction-specific timing curves, slat times, reference direction, direct travel between known positions, reserve, reversal pause, STOP timing, endpoints and timer rollover. |
| `test_shutter_position.py` | Actual position controller: 16-entry FIFO/replacement, queue cancellation/revalidation, target validation, reference/direct movement sequence, unknown-start rejection, same-target no-op, shared status, manual/remote cancellation, calibration ownership/timeout/apply, OTA and invalidation. |
| `test_mqtt_position.py` | Actual MQTT routing and discovery: 0/100 scale, conditional percentage control, stable entity identity, legacy commands, reconnect and maximum field lengths. |
| `test_remote_mapping.py` | Actual mapping code: two-press registration, single/group source matching, shared targets, per-motor removal, Unicode, limits, persistence and migration. |
| `test_serial_migration.py` | Configuration v8 migration through the load path used at boot and for imports, fresh defaults, repeat loads, save failures, recovery and reference-setting persistence. Also checks which base serial inputs the WebUI formats and which the WebSocket accepts. |
| `test_serial_tx.py` | Radio serial compatibility across every 20-bit legacy base and all 16 channels. Checks KeeLoq keys, discriminator values and UP/DOWN/STOP/group frames with representative bases and boundary values. Also exercises invalid channel indices. |
| `test_rx_glitch.py` | Actual edge-capture methods: short HIGH/LOW disturbances, false sync, stale HIGH invalidation, frozen buffers, timer rollover and deterministic noise. |
| `test_rx_decode.py` | Actual capture/decoder/KeeLoq methods: all 16 channels, groups, commands, terminal HIGH handling, truncated frames and discriminator mismatch. |
| `test_ota_validation.py` | `markAppValid` from `src/main.cpp` with a fake clock and OTA API. Checks offline confirmation at 30 seconds, deferral during active OTA, 10-second retries and no repeat after success. |
| `test_position_ui.cjs` | Actual frontend logic: feature/calibration visibility, slider without immediate transmission, explicit execute, calibration steps, apply/cancel, queued target display/replacement and per-channel busy state. |
| `test_remote_mapping_ui.cjs` | Actual frontend logic: selected motor, confirmation before save, Unicode limit, removal, session ownership and cleanup after disconnect. |

The position-model check is compiled by the suite runner. RF tests extract the
relevant methods and fields directly from the library source. They use fixed-width
32-bit arithmetic to match the ESP and synthetic test keys.
If source boundaries change, adapt this harness rather than duplicating production
logic. The other C++ integration checks compile the actual source files against
minimal headers representing the hardware and transport interfaces.

## Limits and maintenance

Passing these checks is not a complete system or hardware test. The UI harness
checks logic, not browser rendering, real WebSocket behavior or accessibility.
The RF checks do not measure RF range, ESP interrupt latency or CC1101 registers
on hardware. The configuration checks simulate filesystem operations and cannot
verify power-loss behavior on an ESP. The MQTT checks do not run a broker or a
real Home Assistant instance. The OTA check simulates calls to the ESP OTA API.
Actual image confirmation and rollback depend on the bootloader and require a
device test. Changes still need an appropriate firmware build and targeted
device tests for calibration, motor movement, radio and OTA where affected.

Keep a test only while it checks a still-valid behavior worth protecting in future
regression or system checks. Update tests when intended behavior changes. Remove
obsolete experiments, duplicate checks and assertions tied only to current visual
layout. Add new maintained tests explicitly to `run_tests.py` and this overview.
