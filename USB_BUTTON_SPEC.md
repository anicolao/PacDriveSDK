# USB Button Specification

This document provides a detailed specification for the Ultimarc USB Button device, based on reverse-engineering the available source code and direct experimentation.

## 1. Overview

The USB Button is a programmable USB device with a single push-button and an RGB LED. It is a composite USB device that presents multiple HID interfaces, but configuration only affects the standard Keyboard interface.

**Conclusion:** The device's firmware can be programmed to send sequences of standard keyboard scancodes. It can send multiple keys simultaneously (chords). However, it **cannot** be programmed to send modifier keys (Ctrl, Shift, Alt) or multimedia keys (Volume, Mute, etc.) via this configuration protocol.

## 2. Device Identification

- **Vendor ID:** `0xd209`
- **Product ID:** `0x1200`
- **Configuration Interface:** The device is configured by sending reports to the HID interface with Usage Page `0x0`.

## 3. Configuration Protocol

Configuration is performed by sending a sequence of HID reports to the Configuration Interface.

### 3.1. Initial Report

A 4-byte report is sent to initiate the configuration sequence.

| Byte 0 | Byte 1 | Byte 2 | Byte 3 |
|---|---|---|---|
| Command | `0xdd` | Mode | Spare |

- **Command:** `0x50` for permanent storage, `0x51` for temporary.
- **Mode:** Determines how the Key Data is interpreted. See section 4.1.

### 3.2. Data Reports

Following the initial report, 15 subsequent 4-byte reports are sent, containing the 60 bytes of configuration data.

## 4. Configuration Data Structure (60 bytes)

| Byte(s) | Description |
|---|---|
| 0-1 | **Spare** |
| 2-4 | **Released Color (RGB)** |
| 5-7 | **Pressed Color (RGB)** |
| 8-61| **Key Data (54 bytes)** |

---

### 4.1. Mode (Set in Initial Report)

- `0x00`: **Alternate Mode**: The Key Data contains two 24-byte chunks of standard keyboard scancodes. The first press sends the first chunk, the second press sends the second chunk.
- `0x02`: **Default Mode**: The Key Data is treated as a sequence of standard keyboard scancodes.

---

### 4.2. Key Data (Bytes 8-61)

This 54-byte buffer contains a sequence of single-byte HID Keyboard Usage IDs (e.g., 'a' is `0x04`, 'F8' is `0x41`). The firmware interprets non-zero bytes in this buffer as keys to be pressed simultaneously. To create a sequence of individual key presses, place one scancode in each 8-byte block of the buffer.

---

### 4.3. Single Key (Hold) Mode

If the configured key data contains only a single non-zero scancode, the device firmware automatically enters a "one key keyboard" mode, holding the key down as long as the button is pressed.
