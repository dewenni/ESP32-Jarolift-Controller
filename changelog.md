**Changelog**

# v1.6.1

## what's new

this is a minor bugfix version

## changelog

- [BUGFIX] URL entries in the settings were not saved #29

---

# v1.6.0

## what's new

this is a minor update and bugfix version

### WebUI-Demo

For a first impression of the functions and the WebUI, a limited demo is also available.  
This can be accessed via the following link: [WebUI-DEMO](https://dewenni.github.io/ESP32-Jarolift-Controller/)

### Changed condition for Setup-Mode

Setup mode is now activated when the ESP is restarted **5** times.
A maximum of 5 seconds may elapse after each restart.

Example: restart 1/5 - wait 2s - restart 2/5 - wait 2s - restart 3/5 - wait 2s - restart 4/5 - wait 2s - restart /5/5 => Setup-Mode


## changelog

- [UPDATE]  ESP32Async/AsyncTCP @ 3.3.5
- [UPDATE]  ESP32Async/ESPAsyncWebServer @ 3.7.0
- [IMPROVE] Added validation for IP input fields in the setup area to avoid wrong syntax #27
- [IMPROVE] Conversion of the storage of the device counter from EEPROM to NVS
- [IMPROVE] remove callback function for internal messages in jarolift-controller library
- [IMPROVE] change "Double-Reset-Detection" to "Multi-Reset-Detection" now it needs 5 restarts within 5s timeout to enter Setup-Mode #26
- [NEW] Add a [WebUI-DEMO](https://dewenni.github.io/ESP32-Jarolift-Controller/) under github-pages
- [NEW] Added some description for possible teach-in procedures.

---

# v1.5.1

## what's new

this is a minor update and bugfix version

## changelog

- [UPDATE]  ESP32Async/AsyncTCP @ 3.3.2
- [UPDATE]  ESP32Async/ESPAsyncWebServer @ 3.6.2
- [UPDATE]  bblanchon/ArduinoJson @ 7.3.0
- [UPDATE]  dewenni/EspSysUtil @ 1.0.1
- [IMPROVE] add button for GitHub-OTA in Tools section #34
- [BUGFIX]  some log messages got lost at startup
- [BUGFIX]  WiFi Status information was not updated in webUI

---

# v1.5.0

## what's new

This update brings a cool new feature 🎉🎉🎉

### 1. Timer Control

The timer function enables the automatic control of individual roller shutters or a selection of several roller shutters as a group.
A fixed time can be specified as a trigger, or sunrise or sunset with an optional time offset.
This controller can therefore also work well as a stand-alone solution without the need for further automation

![webUI_timer](/Doc/webUI_timer.png)

The selection of roller shutters is supported by an additional dialogue. All configured and activated roller shutters are displayed there. These can be selected there and the bitmask is then automatically created from them.

<img style="width: 400px;" src="./Doc/webUI_bitmask_wiz.png">

> [!TIP]
> Maybe it is necessary to clean your browser cache after the update, to be sure that everything works well!

### encrypted Passwords

Passwords are now better protected and stored in encrypted form in the config.json
When updating, the existing passwords are automatically encrypted and saved again.

> [!CAUTION]
> As the passwords are stored in encrypted form after this update, it is no longer possible to switch to an older version without re-entering the passwords after the downgrade

## changelog

- [UPDATE] Arduino core 3.1.1 based on IDF 5.3.2.241224
- [UPDATE] mathieucarbou/AsyncTCP @ 3.3.2
- [UPDATE] mathieucarbou/ESPAsyncWebServer @ 3.6.0
- [FEATURE] new feature to define 6 individual timer to automatically move the shutters #4
- [FEATURE] new wizard for defining bit masks for timers and groups
- [IMPROVE] uploaded config files via "config-upload" are now automatically renamed to config.json, regardless of the original filename.
- [IMPROVE] Improved behavior when the restart button is pressed immediately after a change in the settings.
- [IMPROVE] Passwords are better protected and are stored in encrypted form

---

# v1.4.0

## what's new

### 1. direct GitHub Update 🎉🎉🎉

This update introduces an exciting new feature:  
You can now update to the latest version directly from the WebUI – no need to download the .bin file manually anymore!

Simply click on the version info in the bottom left corner, and a dialog will open. If a new version is available, you can initiate the update right there. It will automatically download and install the latest release from GitHub.

![ota-2](Doc/github_ota.gif)

### 2. new option to enable/disable WIFI

there is a new option to enable/disable WiFi. This could be useful if you use the Ethernet connection and you do not want to use the WiFi in parallel.
The depending switch to enable/disable is also in Settings > WiFi

> [!CAUTION]
> Unfortunately after the update the "enable" will be false and therefore the Controller will be activate the "Setup-Mode" after restart, because WiFi and in most cases Ethernet is now disabled. Then you have to connect to the Accesspoint, go to Settings and activate WiFi. After Reboot everything should be fine again. Sorry, but that's something I can't avoid in this case.

> [!TIP]
> Maybe it is necessary to clean your browser cache after the update, to be sure that everything works well!

## changelog

- add new feature to update directly from GitHub
- add option to enable/disable WIFI in case ETH is used (WiFi will be activated automatically in Setup-Mode) #17
- improve the size of controls at mobile layout

---

# v1.3.1

## what's new

minor changes and housekeeping

> [!NOTE]
> this project is still in an early phase.
> If you like this project, feel free to push the **[Star ⭐️]** button and click **[Watch 👁]** to stay updated.
> If there is any bug, please use the Issue function and let me know.
> If you have any further questions, suggestions or requests, please use the discussion function.

## changelog

- add group commands for telnet interface
- code cleanup (reorganize internal functions and libraries)
- add german description
- bugfix "Build fails on build_webui.py script with FileNotFoundError" #11

---

# v1.3.0

## what's new

This release brings group support for roller shutters. It is now possible to predefine up to 6 shutter groups. These groups can then be controlled with a command via the WebUI, MQTT or Home Assistant.
It is also possible to use a generic MQTT command to specify the selection of roller shutters for a group directly as a bitmask

### predefined Group

To control shutters a group you can use the following mqtt commands.
{UP, OPEN, 0} means, that you can use one of the listed payload commands.

```text

command:    group up
topic:      ../cmd/group/1 ... cmd/group/6
payload:    {UP, OPEN, 0}

command:    group down
topic:      ../cmd/group/1 ... cmd/group/6
payload:    {DOWN, CLOSE, 1}

command:    group stop
topic:      ../cmd/group/1 ... cmd/group/6
payload:    {STOP, 2}

command:    group shade
topic:      ../cmd/group/1 ... cmd/group/6
payload:    {SHADE, 3}

```

### Group with bitmask

You can also use a generic group command and provide the bitmask to select the shutters directly.  
The bitmask is a 16-bit number, with the least significant bit (on the right) representing channel 1.  
A set bit means that the channel belongs to this group.  

**Example**: `0000000000010101` means that channels 1, 3, and 5 belong to this group.

As payload, you can use three different formats to represent the same bitmask:

- **Binary**: `0b0000000000010101`
- **Hex**: `0x15`
- **Decimal**: `21`

```text

command:    group up
topic:      ../cmd/group/up
payload:    {0b0000000000010101, 0x15, 21}

command:    group down
topic:      ../cmd/group/down
payload:    {0b0000000000010101, 0x15, 21}

command:    group stop
topic:      ../cmd/group/stop
payload:    {0b0000000000010101, 0x15, 21}

command:    group shade
topic:      ../cmd/group/shade
payload:    {0b0000000000010101, 0x15, 21}

```

> [!NOTE]
> this project is still in an early phase.
> If you like this project, feel free to push the **[Star ⭐️]** button and click **[Watch 👁]** to stay updated.
> If there is any bug, please use the Issue function and let me know.
> If you have any further questions, suggestions or requests, please use the discussion function.

## changelog

- [FEATURE] highlight active page in the sidebar navigation
- [FEATURE] Implement Group Commands #6
- [FEATURE] HASS: ignore cover state (use optimistic=true) #8

---

# v1.2.0

## what's new

Implemented a command queue for incomming mqtt messages and also for the CC1101 commands.
Commands are now buffered and processed sequentially to prevent timing issues during transmission.

this project is still in an early phase.
If you like this project, feel free to push the **[Star ⭐️]** button and click **[Watch 👁]** to stay updated.
If there is any bug, please use the Issue function and let me know.
If you have any further questions, suggestions or requests, please use the discussion function.

## changelog

- fix missing SW-Version in MQTT-SysInfo and Home Assistant
- Implemented a command queue for incoming mqtt messages. #5
- Implemented a command queue for CC1101 to handle rapid consecutive commands to prevent timing issues during transmission. #5
- update to Arduino core 3.1.0 based on IDF 5.3.2.241210

---

# v1.1.1

## what's new

this is a minor bug fix version!

this project is still in an early phase.
If you like this project, feel free to push the **[Star ⭐️]** button and click **[Watch 👁]** to stay updated.
If there is any bug, please use the Issue function and let me know.
If you have any further questions, suggestions or requests, please use the discussion function.

## changelog

- extend internal check if cc1101 is connected to avoid infinite-loop #2


---

# v1.1.0

## what's new

minor changes and updates!

this project is still in an early phase.
If you like this project, feel free to push the **[Star ⭐️]** button and click **[Watch 👁]** to stay updated.
If there is any bug, please use the Issue function and let me know.
If you have any further questions, suggestions or requests, please use the discussion function.

## changelog

- add gpio check (check for duplicates and set initial values if gpio=0)
- set initial log level to "Debug"
- update mathieucarbou/AsyncTCP @ 3.3.1
- update mathieucarbou/ESPAsyncWebServer @ 3.4.3

---

# v1.0.0

## what's new

this is the initial release of this new project 🎉🎉🎉

If you like this project, feel free to push the **[Star ⭐️]** button and click **[Watch 👁]** to stay updated.

If there is any bug, please use the Issue function and let me know.

If you have any further questions, suggestions or requests, please use the discussion function.
