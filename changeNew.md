# v1.10.0

## what's new

### homee smart home integration

The Jarolift Controller can now be integrated directly into [homee](https://hom.ee/). If enabled in the new "Homee" settings category, every configured shutter is registered as its own virtual homee node, with:

- a shutter switch (up/down/stop)
- a button for the 3rd position ("shade" / intermediate position)
- the firmware revision as an additional attribute

Only shutters that are actually configured (enabled) are registered as nodes. The name of the virtual homee itself is taken from the hostname configured under WiFi and is shown read-only in the settings page.

In the compatibility mode the shader node profile of homee is used which might be more compatible but does not support a third position. The 3rd button mode creates a device with an "unkown" node profile which might be less compatible to specific applications.

If "enable homee" is left disabled, there is no change in behavior compared to before.

## changelog

- [FEATURE] add homee smart home integration (virtual homee node with shutter switch, shade button and firmware revision per configured shutter)
