**Changelog**

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
