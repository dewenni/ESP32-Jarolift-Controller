# v1.7.0

## what's new

### EspWebUI-Library

with this update, the basic WebUI functions were outsourced to a separate [EspWebUI](https://github.com/dewenni/EspWebUI) library.
This should have no effect on the user experience, but it improves the handling of similar projects like [ESP_Buderus_KM271](https://github.com/dewenni/ESP_Buderus_KM271)

### support for received remote signals

As requested by some users, there is now also support for original remote controls. The signals from the remote controls are now captured, processed and also made available via MQTT. Further details can be found in the updated and extended readme.

### support for different ESP32 Chips

There is now also support for other ESP32 chips. This allows you a greater variance in the selection of your hardware.
For the newly supported chips, a firmware file for OTA updates and one for initial flashing will be offered in future. The direct GitHub update via the WebUI should also work for the new chips.

Supported Chips:

- ESP32
- ESP32-S2 (new)
- ESP32-S3 (new)
- ESP32-C3 (new)

### some minor WebUI improvements

- When a shutter command is executed in the WebUI, a message confirming the command is displayed for 2 seconds at the footer.
- Add additional ESP-Chip infos in the system section of the WebUI
- Add add additional error messages at the footer if Jarolift Keys or serial number not set

## changelog

- [UPDATE]  ESP32Async/AsyncTCP @ 3.3.6
- [UPDATE]  ESP32Async/ESPAsyncWebServer @ 3.7.2
- [UPDATE]  bblanchon/ArduinoJson @ 7.3.1
- [CHANGE] Basic WebUI functions were outsourced to a separate [EspWebUI](https://github.com/dewenni/EspWebUI) library
- [FIX] update WiFi and Ethernet Status also if mqtt is not connected
- [FIX] the entry of Jarolift Keys or the serial number was not accepted if you clicked on LOCK directly after the entry
- [FIX] skip websocket connection if WebUI is located on localhost or github.io
- [FEATURE] add additional error messages if Jarolift Keys or serial number not set
- [FEATURE] When a shutter command is executed in the WebUI, a message confirming the command is displayed for 2 seconds at the footer.
- [FEATURE] add support for received remote signals
- [FEATURE] add support for multiple ESP32 Chips
- [FEATURE] add additional ESP-Chip infos in the system section of the WebUI
