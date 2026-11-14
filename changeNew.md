# v2.0.0

Version 2.0.0 is a major release with a completely redesigned WebUI, calibrated percentage positioning and a new way to associate original remote controls with shutters. Radio reception has been significantly improved, alongside more reliable connections, updates and configuration storage.

## Highlights

### Calibrated shutter positioning

Set an opening target from **0% closed to 100% open** in the WebUI, via MQTT or through Home Assistant. Enable positioning globally under **Settings → Device**, then calibrate each desired shutter with the guided assistant on the **Service** page. Shutters without calibration remain usable with the normal UP, STOP, DOWN and SHADE buttons.

Separate opening and closing measurements account for the initial take-up time, the final slat closure and the changing winding diameter. Every percentage command first performs a full reference run to an endpoint, then moves to the requested opening. This avoids relying on a remembered position that may have changed through an original remote or a local wall switch.

All positions remain **time-based estimates without motor feedback**. Receiving original remote commands helps keep the status consistent, but cannot replace the reference run. The channel widgets show the estimated position with a compact shutter graphic and a separate target slider and execution button.

MQTT percentage targets use `<prefix>/cmd/position/N` with an integer payload from 0 to 100, without retain. Home Assistant Discovery adds percentage controls only for globally enabled, calibrated shutters and updates them when eligibility changes. WebUI, MQTT and Home Assistant share a fixed RAM queue for up to **16 waiting targets**, processed sequentially after the active run. New targets replace an existing waiting target for the same shutter. The WebUI marks pending targets with a spinner in the execute button and a compact queued-target label; the queue position is available in the button tooltip. Manual commands discard waiting targets; calibration remains exclusive. Group percentage positioning is not included.

See [Positioning and calibration](Doc/Positionierung.md) for the operating principle, calibration steps and MQTT/Home Assistant examples.

### Original remote controls: capture and associate per shutter

Original remotes are now managed directly on **Service** for the selected shutter. Capture an already paired single channel, confirm it with a second separate button press and give it a name. No remote model, manually entered serial number or channel mask is required.

- Up to **five associations per shutter**, with up to 80 distinct sources overall.
- Source names support up to **32 Unicode characters**.
- One source can be associated with multiple ESP shutters.
- Group commands automatically resolve the registered individual source channels; no separate group registration is needed.
- Removing an association affects only the selected shutter's ESP mapping, not the actual pairing at the motor.

Assigned remote commands update the shared WebUI/MQTT position estimate and cancel conflicting automatic movement sequences. Capturing a source does not transmit a motor pairing command; operating the original remote can still move its paired shutters.

### Significantly improved radio reception and more consistent transmission

The receive path has been improved through tuned CC1101 settings, better pulse validation and a **200 µs glitch filter** that merges short interference spikes. Frame completion and the final channel bit are now handled correctly, including **channel 16**. Incomplete frames and inconsistent discriminator values are rejected rather than passed on as apparent shutter commands.

Transmission now uses the ESP's **RMT peripheral** for consistent pulse timing. The configured transmit power is applied before CC1101 initialization. Detailed RF diagnostics are available at DEBUG level, while received commands remain visible at INFO level.

These changes substantially improve reception in practice, but do not guarantee reception of every remote or provide motor acknowledgements. See [RF reception](Doc/RF-Empfang.md) for the protocol and implemented filters.

### New WebUI style and clearer navigation

The interface has a new grey-blue theme with restrained warm accents, clearer typography and more consistent controls, tables and status displays. Desktop and mobile navigation, the network-status footer and the project-specific login page have also been revised.

Settings are organised into **General**, **Network**, **Integrations**, **Device**, and **Access & maintenance**. Related sections are kept together, with only one subsection expanded at a time. Home Assistant has its own integration section with a reminder that MQTT is required.

The **Tools** page clearly separates local firmware files from GitHub updates. GitHub version information and update execution now stay on the page instead of opening a separate version dialog.

The revised **Help** page provides German and English instructions for shutter operation, groups and timers, setup, positioning, original remotes, MQTT and connection reference information. Expandable sections make the guidance easier to navigate without leaving the device.

### WiFi network and access-point search

Separate search buttons list available **SSIDs** and **access points/BSSIDs** in selection dialogs. Hidden SSIDs can still be entered manually. Leave BSSID empty for automatic access-point selection, or select a fixed access point. Changing the SSID clears the previous BSSID binding.

### homee integration

Configured shutters can be exposed directly as virtual homee nodes with UP/DOWN/STOP, an optional SHADE button and firmware revision information. Enable and configure homee under **Settings → Integrations**. Compatibility mode uses the standard shutter profile; the optional third position uses an unknown node profile, which may be less compatible with some applications. This integration does not add calibrated percentage control to homee.

## Breaking changes and upgrade notes

### MQTT position status now means opening percentage

**`<prefix>/status/shutter/1` … `/16` now use 0 = fully closed and 100 = fully open. The previous scale was reversed.** Update manual MQTT cover configurations, numeric comparisons, automations and dashboards accordingly.

Intermediate values are estimated opening percentages. An unknown position is published as retained **`None`**, for example after restart, STOP or SHADE. No arbitrary intermediate shade position is assumed. MQTT reconnect republishes the current status to replace stale retained values.

The existing command identifiers are unchanged: on `cmd/shutter/N`, **`0` still means UP, `1` DOWN and `2` STOP**. They are not percentage values. Prefer `OPEN`/`CLOSE`/`STOP` where appropriate. On `cmd/position/N`, `0` means fully closed.

Refresh Home Assistant Discovery after upgrading existing installations. Discovery uses the new opening scale and is also refreshed on MQTT reconnect and when positioning eligibility changes. Unknown positions are not actual measurements; Home Assistant may retain its last known numeric position.

### Original remote associations have moved to Service

The old six-digit serial-prefix registration has been removed. Register the already paired **individual channels** on Service for the relevant shutters; groups are matched automatically.

Configuration version 7 migrates version-6 single-channel mappings and merges duplicate sources. Old explicit group mappings remain visible but inactive and must be replaced by capturing their individual channels. Older prefix-only registrations cannot be recovered and must be captured again. Physical motor pairing is unaffected. Imported associations above the new five-per-shutter limit are retained, but must be reduced before adding more to that shutter.

### WebUI and service credentials are separate

- **WebUI:** freely chosen username/password with optional authentication in normal operation.
- **Setup Mode:** fixed login **`admin` / `jarolift-2026!`**, independent of a missing or damaged configuration.
- **Telnet and remote ArduinoOTA:** use a separately configured, shared **service password**. Both remain disabled in normal operation while this password is empty. Telnet must additionally be enabled explicitly.
- **WebUI firmware updates and configuration import/export:** follow WebUI authentication and do not request an additional service password.

Old configurations without service settings leave Telnet and remote ArduinoOTA disabled. Configure the new service password under **Settings → Access & maintenance → Service access** if needed. For PlatformIO remote uploads, pass it with `upload_flags = --auth=<service-password>`.

In Setup Mode, remote ArduinoOTA uses `jarolift-2026!`. Telnet uses the same fixed password only if its enable setting is active; Setup Mode does not automatically enable Telnet. Disabling WebUI authentication does not disable service-password protection.

Back up the configuration before upgrading. Existing motor end positions do not replace the new positioning calibration; calibrate each shutter for which percentage control is desired.

## Further improvements

- Increased OTA update stability.
- More robust MQTT reconnects with increasing retry delays and recovery of the MQTT transport, without rebooting simply because the broker is unavailable.
- Improved WiFi recovery and WebSocket reconnection, including browser background-tab handling.
- Safer configuration saving through verified temporary files, retries and recovery after interrupted writes.
- Queued WebUI command processing, validation of MQTT payloads and fixes for configuration values containing formatting characters.
- Corrected motor learning-mode handling and validation of learning channels, with transmit serial numbers limited to the protocol's bit width.
- Improved validation of configuration values, including language selection, and immediate application of access settings after configuration imports.
- More useful system memory information, connection indicators and a clearer Logger interface with log-level colours and automatic refresh when opened.
- ESP32-S2 retains 128 diagnostic log entries instead of 200 to fit its internal RAM budget; other ESP32 variants retain 200 entries.
- Updated English and German documentation, screenshots and maintained host regression tests.

## Platform and library updates

- Arduino-ESP32 **3.3.12**, based on ESP-IDF **5.5.5** (pioarduino platform **55.03.312-1**).
- **EspWebUI 0.0.13**: shared settings navigation, themed login page and extended WebSocket integration.
- **ESPAsyncWebServer 3.12.1** and **AsyncTCP 3.6.1**.
- **ArduinoJson 7.4.3**.
- **EspStrUtil 1.1.0** and **EspSysUtil 1.1.0**.
- **homee-api-esp32 10.1.0** for the new homee integration.

## Thanks

Thanks to [@Oxi75](https://github.com/Oxi75) for the homee integration ([#79](https://github.com/dewenni/ESP32-Jarolift-Controller/pull/79)), [@kqfflo](https://github.com/kqfflo) for the RX/TX and learning fixes ([#90](https://github.com/dewenni/ESP32-Jarolift-Controller/pull/90)), and [@Reinhard-M](https://github.com/Reinhard-M) for the RF analysis, RMT and filtering work adapted from the [PositionController fork](https://github.com/Banabas/ESP32-Jarolift-PositionController), including [#20](https://github.com/Banabas/ESP32-Jarolift-PositionController/pull/20) and [#21](https://github.com/Banabas/ESP32-Jarolift-PositionController/pull/21).
