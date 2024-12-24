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
