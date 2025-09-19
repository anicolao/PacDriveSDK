# USB Button Specification

This document provides a detailed specification for the Ultimarc USB Button device. It is intended for developers who want to create software to control and configure the USB Button.

## 1. Overview

The USB Button is a programmable USB device with a single push-button and an RGB LED. It can be configured to send a sequence of keystrokes when pressed and to change its LED color based on its state (pressed or released).

## 2. Device Identification

The USB Button can be identified on the USB bus using the following identifiers:

- **Vendor ID:** `0xd209` (Ultimarc)
- **Product ID:** `0x1200`
- **Device Type ID:** `8` (as returned by the `PacGetDeviceType` function in the PacDrive SDK)

## 3. API Functions

The following functions are available for interacting with the USB Button via the PacDrive SDK.

### `bool USBButtonConfigurePermanent(int id, byte[] data)`

Programs the USB Button with the specified configuration data and stores it in the device's non-volatile memory. This configuration will be retained even when the device is powered off.

- **`id`**: The device ID of the USB Button.
- **`data`**: A 62-byte array containing the configuration data. See section 4 for a detailed breakdown of this data structure.
- **Returns**: `true` on success, `false` on failure.

### `bool USBButtonConfigureTemporary(int id, byte[] data)`

Programs the USB Button with the specified configuration data. This configuration is stored in the device's volatile memory and will be lost when the device is powered off. It has been reported that this command may not work on all device firmwares.

- **`id`**: The device ID of the USB Button.
- **`data`**: A 62-byte array containing the configuration data. See section 4 for a detailed breakdown of this data structure.
- **Returns**: `true` on success, `false` on failure.

### `bool USBButtonConfigureColor(int id, byte red, byte green, byte blue)`

Sets the color of the USB Button's RGB LED.

- **`id`**: The device ID of the USB Button.
- **`red`**: The red component of the color (0-255).
- **`green`**: The green component of the color (0-255).
- **`blue`**: The blue component of the color (0-255).
- **Returns**: `true` on success, `false` on failure.

### `bool USBButtonGetState(int id, out bool state)`

Gets the current state of the USB Button.

- **`id`**: The device ID of the USB Button.
- **`state`**: A boolean value that will be set to `true` if the button is currently pressed, and `false` if it is released.
- **Returns**: `true` on success, `false` on failure.

## 4. Configuration Data Structure

The `USBButtonConfigurePermanent` and `USBButtonConfigureTemporary` functions use a 62-byte data array to configure the button's behavior. The structure of this array is as follows:

| Byte(s) | Description |
|---|---|
| 0 | **Mode** |
| 1 | **Spare** |
| 2-4 | **Released Color (RGB)** |
| 5-7 | **Pressed Color (RGB)** |
| 8-31| **Text 1 (24 bytes)** |
| 32-55| **Text 2 (24 bytes)** |
| 56-61| **Spare (6 bytes)** |

---

### 4.1. Byte 0: Mode

This byte determines the button's operational mode.

- `0x00`: **Alternate Mode** - On first press, sends keys from Text 1. On second press, sends keys from Text 2.
- `0x01`: **Extended Mode** - On a short press, sends keys from Text 1. On a long press, sends keys from Text 2.
- `0x02`: **Default Mode** - Sends the entire 48+ key sequence from Text 1 and Text 2 on every press.

*Note: The C# source code comment for mode `02` says "both", which is interpreted as the default "send all keys" mode.*

---

### 4.2. Bytes 1, 56-61: Spare

These bytes are currently unused and should be set to `0x00`.

---

### 4.3. Bytes 2-4: Released Color (RGB)

These three bytes define the color of the RGB LED when the button is in the **released** state.

- **Byte 2**: Red component (0-255)
- **Byte 3**: Green component (0-255)
- **Byte 4**: Blue component (0-255)

---

### 4.4. Bytes 5-7: Pressed Color (RGB)

These three bytes define the color of the RGB LED when the button is in the **pressed** state.

- **Byte 5**: Red component (0-255)
- **Byte 6**: Green component (0-255)
- **Byte 7**: Blue component (0-255)

---

### 4.5. Bytes 8-55: String Data

These bytes represent the string(s) of characters that the USB Button will send as keystrokes. The string data is split into two 24-byte chunks.

- **Bytes 8-31**: **Text 1**. Used for the first press in Alternate mode, the short press in Extended mode, or the first half of the sequence in Default mode.
- **Bytes 32-55**: **Text 2**. Used for the second press in Alternate mode, the long press in Extended mode, or the second half of the sequence in Default mode.

The string is encoded as a sequence of bytes, where each byte corresponds to a USB HID Usage ID for a keyboard character.

| Character | HID Usage ID (Decimal) | HID Usage ID (Hex) |
|---|---|---|
| 'A' - 'Z' | 4 - 29 | 0x04 - 0x1D |
| Space | 44 | 0x2C |

*Note: The character-to-byte mapping is `(byte)(character - 'A' + 4)`. The string should be converted to uppercase before encoding. Any unused bytes should be padded with zeros.*

### 4.6. Single Key (Hold) Mode

If the configured text is a single character (e.g., "a"), the device firmware will automatically enter a "one key keyboard" mode. In this mode, the key will be held down as long as the button is pressed, and released when the button is released.

### 4.7. Modifier Keys (e.g., Ctrl, Shift)

The method for encoding modifier keys is not documented in the available source code and is currently not supported by this specification.
