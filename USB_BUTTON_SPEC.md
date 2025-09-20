# USB Button Specification

This document provides a detailed specification for the Ultimarc USB Button device, based on reverse-engineering the available source code and direct experimentation.

## 1. Overview

The USB Button is a programmable USB device with a single push-button and an RGB LED. It is a composite USB device that presents multiple HID interfaces to the operating system, including a Keyboard interface and a Consumer Control interface. It can be configured to send a sequence of keystrokes or multimedia commands, and to change its LED color based on its state.

Configuration is sent to a specific configuration interface on the device.

## 2. Device Identification

- **Vendor ID:** `0xd209`
- **Product ID:** `0x1200`
- **Interfaces:**
    - **Configuration Interface:** Usage Page `0x0`
    - **Keyboard Interface:** Usage Page `0x01` (Generic Desktop), Usage `0x06` (Keyboard)
    - **Consumer Control Interface:** Usage Page `0x0C` (Consumer)
    - **System Control Interface:** Usage Page `0x01` (Generic Desktop), Usage `0x80` (System Control)

## 3. Configuration Protocol

Configuration is performed by sending a sequence of HID reports to the **Configuration Interface** (`usage_page = 0`).

### 3.1. Initial Report

A 4-byte report is sent to initiate the configuration sequence.

| Byte 0 | Byte 1 | Byte 2 | Byte 3 |
|---|---|---|---|
| Command | `0xdd` | Mode | Spare |

- **Command:** `0x50` for permanent (non-volatile) storage, `0x51` for temporary.
- **Mode:** Determines how the Key Data is interpreted. See section 4.1.
- **Spare:** Unused, set to `0x00`.

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
- `0x01`: **Extended Mode**: The Key Data is treated as a sequence of single-byte scancodes from the Consumer HID Page (e.g., for multimedia keys).
- `0x02`: **Default Mode**: The Key Data is treated as a sequence of standard keyboard scancodes.
- `0x03`: **Macro Mode**: The Key Data is treated as a sequence of up to six 8-byte chunks. Each chunk represents a set of keys to be pressed simultaneously.

---

### 4.2. Key Data (Bytes 8-61)

This 54-byte buffer's interpretation depends on the **Mode**.

- **Default/Alternate Modes:** Contains single-byte HID Keyboard Usage IDs (e.g., 'a' is `0x04`).
- **Extended Mode:** Contains single-byte HID Consumer Page Usage IDs (e.g., Volume Up is `0xE9`).
- **Macro Mode:** Contains a sequence of up to six 8-byte chunks. Each chunk is a list of up to 6 simultaneous key presses (plus 2 spare bytes). To send a modifier key like `Ctrl`, its HID Usage ID (`0xE0`) is included in the chunk along with the other key(s).
    - **Example `Ctrl+R`:** A chunk of `E0:15:00:00:00:00:00:00` would press Left Control and R simultaneously. A subsequent "all keys up" chunk (`00:00...`) is required to release them.

---

### 4.3. Single Key (Hold) Mode

If the configured text in Default Mode is a single character, the device firmware automatically enters a "one key keyboard" mode, holding the key down as long as the button is pressed.
