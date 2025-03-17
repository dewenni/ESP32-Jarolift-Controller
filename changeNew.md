# v1.9.0

## what's new

### Service Commands

there is a new WebUI Page with service commands like:

- set shade
- set/delete upper end point
- set/delete lower end point

### remote signals (update)

The MQTT message for remote signals has been updated. The information about which shutter is controlled can be seen in the two variables `chBin` and `chDec`. `chBin` shows the used shutter as a 16-bit binary value, while `chDec` shows the same information as a decimal value for easier automation.

```json
topic:      ../status/remote/<serial-number>
payload:    {
              "name":   "<alias-name>", 
              "cmd":    "<UP, DOWN, STOP, SHADE>",
              "chBin":  "<channel-binary>",
              "chDec":  "<channel-decimal>"
            }
```

## changelog

- [FIX] bugfix github ota asset check #40, #45
- [FEATURE] service page with new service commands
- [FEATURE] new "unlearn" command in shutter settings
- [CHANGE] "set shade" was moved from settings to service page
- [CHANGE] MQTT messages for remote signals have been updated
- [CHANGE] internal redesign in JaroliftController-Lib
- [UPDATE] dewenni/EspWebUI @ 0.0.4
