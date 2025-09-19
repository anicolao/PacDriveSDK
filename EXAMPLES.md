# USB Button Tool Examples

This document provides examples for the `usbbutton_tool`, which allows you to configure the USB Button's colors and key press actions.

**Note:** You may need to run these commands with `sudo` depending on your system's USB device permissions. All configuration is done with the `--configure` command.

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

### Example 2: Extended Mode (Multimedia Keys)

This command configures the button to send multimedia key commands. The `extended` mode tells the button to interpret the key codes as multimedia keys.

**Command:**
```bash
sudo ./usbbutton_tool --configure --permanent --mode extended --keys "volume_up,volume_down,mute" --released-color 0,255,0 --pressed-color 255,0,0
```

**Expected Effect:**
*   The button's LED should be green.
*   The first press should increase the system volume.
*   The second press should decrease the system volume.
*   The third press should mute/unmute the system volume.

---

### Example 3: Macro Mode (Ctrl+R)

This command configures the button to send a `Ctrl+R` key combination. The `macro` mode tells the button to interpret the data as raw 8-byte HID keyboard reports.

**Format:** `MODIFIER:RESERVED:KEY1:KEY2:KEY3:KEY4:KEY5:KEY6`

**Command:**
```bash
sudo ./usbbutton_tool --configure --permanent --mode macro --macro "01:00:15:00:00:00:00:00,00:00:00:00:00:00:00:00" --released-color 0,255,0 --pressed-color 255,0,0
```

**Breakdown of the `--macro` string:**
*   `01:00:15:00:00:00:00:00`: This is the "press" report.
    *   `01`: Modifier byte for Left Control.
    *   `15`: HID usage ID for the 'r' key.
*   `,` : Separates the press report from the release report.
*   `00:00:00:00:00:00:00:00`: This is the "release" report (all keys and modifiers are up).

**Expected Effect:**
*   The button's LED should be green.
*   When you press the button, it should send a `Ctrl+R` key combination.

---

### Example 4: Single Key (Hold) Mode

This command configures the button to act like the 'a' key on a keyboard. This is a special case of `default` mode.

**Command:**
```bash
sudo ./usbbutton_tool --configure --permanent --mode default --text "a" --released-color 0,255,0 --pressed-color 255,0,0
```

**Expected Effect:**
*   The button's LED should be green.
*   When you press and hold the button, it should act as if you are holding down the 'a' key (e.g., it should type 'aaaaaaaaa...').
*   When you release the button, the 'a' key should also be released.
