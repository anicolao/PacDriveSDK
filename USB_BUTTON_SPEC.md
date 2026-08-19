# Ultimarc USB Button configuration protocol

This specification is based on the vendor SDK, the official USB Button manual,
and the decompiled 2016 `USB_Button.exe` configuration utility. The utility is
important because the older SDK exposes only an opaque byte array and its sample
GUI does not implement the complete key grid.

## Device and interface

- USB vendor ID: `0xd209`
- USB product ID: `0x1200`
- Configuration HID interface: usage page `0x01`, usage `0x00`
- Keyboard HID interface: usage page `0x01`, usage `0x06`

Configuration reports must be sent to the usage-0 interface, not to the
keyboard interface. An unnumbered output report is five bytes as seen by
HIDAPI: a zero report ID followed by four payload bytes.

## 64-byte configuration

The complete configuration is divided into sixteen four-byte output reports.

| Byte(s) | Meaning |
|---|---|
| 0 | Command: `0x50` permanent, `0x51` temporary |
| 1 | Protocol marker `0xdd` |
| 2 | Action mode |
| 3 | Bluetooth settings/reserved; official utility defaults to `0x11` |
| 4-6 | Released RGB color |
| 7-9 | Pressed RGB color |
| 10-33 | Primary key sequence: four rows of six cells |
| 34-57 | Secondary key sequence: four rows of six cells |
| 58-62 | Reserved |
| 63 | BluButton LED time; unused by the USB model |

The action modes are:

- `0x00` — **Alternate:** primary and secondary on alternate presses.
- `0x01` — **Extended:** primary followed by secondary on every press.
- `0x02` — **Both:** primary on a short press, secondary on a long press.

## Key rows and modifiers

Each sequence is a 4x6 grid. Cells are processed left-to-right and then
top-to-bottom. A modifier in a row remains held through the later cells in that
row and is released at the row boundary. For example, Ctrl+W is encoded in one
row as:

```text
70 1a 00 00 00 00
```

Ordinary keys use USB HID Keyboard Usage IDs (`W` is `0x1a`), but the device's
stored modifier codes are private values:

| Key | Stored value | Standard HID usage |
|---|---:|---:|
| Left Ctrl | `0x70` | `0xe0` |
| Left Shift | `0x71` | `0xe1` |
| Left Alt | `0x72` | `0xe2` |
| Left GUI/Win | `0x73` | `0xe3` |
| Right Ctrl | `0x74` | `0xe4` |
| Right Shift | `0x75` | `0xe5` |
| Right Alt | `0x76` | `0xe6` |
| Right GUI/Win | `0x77` | `0xe7` |

This translation is why writing `0xe0` directly did not produce Ctrl in the
earlier reverse-engineered tool. The official manual explicitly supports
modifier chords such as Ctrl+Alt+Delete.

## Reading and verification

Send the four-byte payload `59 dd 00 00` to request the stored configuration.
The device replies with sixteen four-byte input reports. The official utility
may receive and discard an initial all-zero report, waits for the response, and
compares configuration bytes 2 through 61 after programming.

## Sources

- [Official USB Button manual](https://www.usbbutton.com/docs/USBButton.pdf)
- `dll/PacDrive.cpp` in this repository for the older write protocol
- Official `USB_Button.exe` configuration utility, particularly its decompiled
  `KeyClass` and `DeviceManager` classes
