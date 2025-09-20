# USB Button Tool Examples

This document provides the final, working examples for the `usbbutton_tool`.

**Note:** You may need to run these commands with `sudo` depending on your system's USB device permissions.

---

### Example 1: Default Mode (Send Text)

This command configures the button to type "hello" when pressed.

**Command:**
```bash
sudo ./usbbutton_tool --configure --permanent --mode default --text "hello" --released-color 0,255,0 --pressed-color 255,0,0
```

**Expected Effect:**
*   The button's LED should be green.
*   When you press the button, it should type "hello".

---

### Example 2: Extended Mode (Multimedia Keys) - Should Work Now

This command configures the button to send a **Volume Up** command. The `extended` mode (`mode=1`) tells the button to interpret the key data as multimedia key scancodes.

**Command:**
```bash
sudo ./usbbutton_tool --configure --permanent --mode extended --keys "volume_up" --released-color 0,255,0 --pressed-color 255,0,0
```

**Expected Effect:**
*   The button's LED should be green.
*   When you press the button, the system volume should increase. You can verify this with `evtest` on the **Consumer Control** interface (`/dev/input/event10`).

---

### Example 3: Macro Mode (Ctrl+R) - The Correct Version

This command configures the button to send a `Ctrl+R` key combination. We now know the key data buffer is a list of keys to press simultaneously.

**Command:**
```bash
sudo ./usbbutton_tool --configure --permanent --mode macro --macro "E0:15:00:00:00:00:00:00,00:00:00:00:00:00:00:00" --released-color 0,255,0 --pressed-color 255,0,0
```

**Breakdown of the `--macro` string:**
*   `E0:15:00:00:00:00:00:00`: This is the "press" report.
    *   `E0`: The HID Usage ID for **Left Control**.
    *   `15`: The HID Usage ID for the 'r' key.
*   `,` : Separates the press report from the release report.
*   `00:00:00:00:00:00:00:00`: This is the "release" report (all keys up).

**Expected Effect:**
*   The button's LED should be green.
*   When you press the button, it should send a `Ctrl+R` key combination. You can verify this with `evtest` on the **Keyboard** interface (`/dev/input/event11`). You should see events for `KEY_LEFTCTRL` and `KEY_R` being pressed and released together.
