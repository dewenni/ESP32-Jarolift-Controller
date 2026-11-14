# Unreleased

## Breaking change: original remote mappings

Original remote associations are now managed in Service for the selected shutter, without a remote model or manual serial/mask entry. Capture and confirm an already paired **single channel**, then name it (up to 32 Unicode characters). Up to five associations per shutter and 80 distinct sources are stored in configuration version 7. Groups automatically match their registered source bits within the same sender family (`serial & 0x0ffffff0`). Removing an association affects only the selected shutter. Existing v6 single-channel entries are merged by source; old group mappings remain visible but inactive and must be captured again as single channels. Older prefix mappings cannot be recovered. Physical motor pairing is unaffected. Capture does not transmit motor commands, but pressing the original remote may operate the shutters.

## Home Assistant percentage control

Eligible individual shutters expose `<prefix>/cmd/position/N` for integer opening targets 0..100. These use the same reference run and calibrated timing as the WebUI. Disabled, uncalibrated, busy or unavailable channels reject percentage commands. Publish commands without retain. Group percentage control is not included.

## Breaking change: unified shutter opening status

`<prefix>/status/shutter/1` … `/16` now use **0 = fully closed, 100 = fully open** (previously reversed). Intermediate numbers are estimated opening percentages. Update manual MQTT cover configurations, automations and dashboards; invert old numeric comparisons. Command payloads remain unchanged: `0` still means UP, `1` DOWN, `2` STOP; prefer OPEN/CLOSE/STOP to avoid confusion.

WebUI and MQTT now share one position estimate, including received commands from assigned original remotes and group commands. For calibrated shutters, manual UP/DOWN assumes the endpoint only after the full calibrated travel time plus reserve; repeated frames do not restart this timer. STOP, SHADE and interrupted automatic moves invalidate the estimate: retained MQTT payload `None` replaces stale positions. There is no assumed 10/90% shade position. Uncalibrated shutters still report the expected endpoint immediately for UP/DOWN. All values are estimates without motor feedback; missed remote signals remain undetectable.

MQTT reconnect republishes the current estimates (or `None` after restart) so old retained values cannot retain the previous scale. Home Assistant Discovery is adjusted to the new status definition; resend discovery after updating existing installations. Home Assistant additionally gets a percentage slider only for globally enabled, calibrated channels. Discovery is refreshed automatically on eligibility changes and MQTT reconnect. OPEN/CLOSE/STOP remain available for all configured shutters.

# v1.10.0

## what's new

After a long absence, here's a major update with new features and improvements.

### homee smart home integration

Thanks to [@Oxi75 (Andreas)](https://github.com/Oxi75) for contributing the homee integration in [PR #79](https://github.com/dewenni/ESP32-Jarolift-Controller/pull/79).

The Jarolift Controller can now be integrated directly into [homee](https://hom.ee/). If enabled in the new "Homee" settings category, every configured shutter is registered as its own virtual homee node, with:

- a shutter switch (up/down/stop)
- an optional button for the 3rd position ("shade" / intermediate position)
- the firmware revision as an additional attribute

Only shutters that are actually configured (enabled) are registered as nodes. The name of the virtual homee itself is taken from the hostname configured under WiFi and is shown read-only in the settings page.

In compatibility mode, the standard shutter node profile of homee is used without a third position. Enabling the 3rd position creates a device with an "unknown" node profile, which may be less compatible with some applications.

If "enable homee" is left disabled, there is no change in behavior compared to before.

### password protection for Telnet and ArduinoOTA

Telnet connections and PlatformIO wireless uploads now require the password configured in the WebUI Auth settings, even when web authentication is disabled. If no password is configured, the shared fallback password is `jarolift-2026!`; set your own password in the WebUI. The same fallback is also used by the WebUI when authentication is enabled without a configured password.

For PlatformIO OTA uploads, supply `upload_flags = --auth=<password>` in addition to the device's `upload_port`. Telnet requires authentication again after reconnecting and disconnects clients that enter a wrong password.

### more reliable updates, connections and configuration storage

Browser firmware uploads are aborted after 60 seconds without progress, restoring the previous watchdog state. Network reconnects and related reboots are suspended during OTA updates. On installations with bootloader rollback support, new firmware is confirmed after two minutes of uptime with a network connection, or after ten minutes offline. A reboot before confirmation can trigger rollback to the previous firmware.

An unavailable MQTT broker no longer causes the controller to reboot. Reconnect attempts use an increasing delay of up to 60 seconds. WiFi recovery resets the WiFi stack periodically and only reboots as a last resort after about 30 minutes without a network connection.

Configuration changes are written to a temporary file and verified before replacing the previous configuration. Failed saves are retried. If the main configuration cannot be read at startup, a valid temporary save or upload file can be recovered.

### radio reception, learning and WebUI improvements

Thanks to [@kqfflo](https://github.com/kqfflo) for contributing the radio reception, TX serial packing and learning mode fixes in [PR #90](https://github.com/dewenni/ESP32-Jarolift-Controller/pull/90).

Radio frame capture and decoding have been corrected, including handling incomplete frames and protecting the receive buffer while decoding. Transmitted serial numbers are limited to the protocol's bit width so they cannot overwrite command bits. The selected learning mode is now preserved and applied correctly, with additional diagnostics for reception and learning.

The Logger page automatically refreshes when opened and displays messages in different colors depending on their log level. Settings now include English and German descriptions for homee, GPIOs, shutters, groups and remotes.

## changelog

- [FEATURE] add homee smart home integration with up/down/stop, optional shade button and firmware revision per enabled shutter ([#79](https://github.com/dewenni/ESP32-Jarolift-Controller/pull/79) by [@Oxi75](https://github.com/Oxi75))
- [FEATURE] require password authentication for Telnet and ArduinoOTA using the shared WebUI Auth password, with a fallback when no password is configured
- [FEATURE] defer OTA firmware confirmation to allow bootloader rollback before validation on supported installations
- [FIX] correct radio RX frame capture, synchronization between interrupt and loop, pulse validation and frame completion ([#90](https://github.com/dewenni/ESP32-Jarolift-Controller/pull/90) by [@kqfflo](https://github.com/kqfflo))
- [FIX] prevent TX serial bits from overwriting command bits and limit the base serial to 20 bits (#90 by @kqfflo)
- [FIX] preserve and apply the selected learning mode, reject invalid learning channels and correct repeated-stop detection (#90 by @kqfflo)
- [FIX] handle missing radio callbacks safely and initialize the radio in receive mode
- [FIX] treat WiFi and Ethernet settings as literal strings instead of format strings, including values containing percent characters
- [FIX] clamp language indices from configuration files and WebUI input to the supported range
- [FIX] preserve default authentication and logger settings when initializing a new configuration
- [FIX] reject invalid, oversized or fragmented MQTT commands instead of processing incomplete payloads
- [FIX] queue WebUI commands safely so consecutive commands do not overwrite each other before processing
- [FIX] abort stalled browser OTA uploads after 60 seconds without progress and restore the previous watchdog state
- [FIX] prevent overlapping OTA update paths and suspend WebUI command processing, MQTT processing and network reconnects during updates
- [FIX] keep the watchdog disabled in setup mode after GitHub OTA updates
- [FIX] apply authentication settings immediately after configuration imports and keep ArduinoOTA synchronized with password changes
- [FIX] save configuration through a verified temporary file, retain the previous file on failure and retry failed saves
- [FIX] recover valid temporary configuration or upload files when the main configuration is missing or invalid
- [IMPROVE] reconnect to MQTT with increasing delays up to 60 seconds without rebooting on broker outages
- [IMPROVE] recover WiFi with periodic stack resets, preserve static IP settings and delay the last-resort reboot until about 30 minutes offline
- [IMPROVE] automatically refresh the Logger page when opened and colorize messages by log level
- [IMPROVE] add English and German descriptions for homee, GPIO, shutter, group and remote settings
- [IMPROVE] add radio reception and learning diagnostics, with cryptographic key logging restricted to an explicit debug build option
- [UPDATE] Arduino Release v3.3.12 based on ESP-IDF v5.5.5
- [UPDATE] ESP32Async/ESPAsyncWebServer @ 3.12.1
- [UPDATE] ESP32Async/AsyncTCP @ 3.5.0
- [UPDATE] bblanchon/ArduinoJson @ 7.4.3
- [UPDATE] dewenni/EspWebUI @ 0.0.7 and refresh generated WebUI assets
- [UPDATE] add Oxi75/homee-api-esp32 @ 10.1.0
- [CHANGE] use ESP32 as the default PlatformIO build target and update the serial upload port example
- [CHANGE] add ESP32-C3 USB mode and USB CDC build flags
- [CHANGE] set the internal development version to v1.9.900 in preparation for v1.10.0
- [DOCS] document Telnet and OTA authentication, upload timeout and firmware rollback behavior
- [DOCS] add and update the stargazer chart in the English and German READMEs
