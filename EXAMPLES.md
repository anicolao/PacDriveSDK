# USB Button Tool Examples

This document provides examples for the final, working version of the `usbbutton_tool`.

**Conclusion from our testing:** The device firmware only supports programming standard keyboard key sequences. It does not support modifier keys (like Ctrl) or multimedia keys via this configuration method.

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

### Example 2: Alternate Mode

This command configures the button to type "first" on the first press, and "second" on the second press.

**Command:**
```bash
sudo ./usbbutton_tool --configure --permanent --mode alternate --text1 "first" --text2 "second" --released-color 0,255,0 --pressed-color 255,0,0
```

**Expected Effect:**
*   The button's LED should be green.
*   On the first press, it should type "first".
*   On the second press, it should type "second".

---

### Example 3: Multi-Key Press (Chord)

This command configures the button to press the 'F8' and 'R' keys simultaneously. This is useful for mapping to hotkeys in applications.

**Command:**
```bash
sudo ./usbbutton_tool --configure --permanent --mode default --hex-codes "41,15" --released-color 0,255,0 --pressed-color 255,0,0
```

**Breakdown of the `--hex-codes` string:**
*   `41`: The HID Usage ID for the `F8` key.
*   `15`: The HID Usage ID for the `R` key.

**Expected Effect:**
*   The button's LED should be green.
*   When you press the button, it should send `F8` and `R` key presses at the same time.

---

### Example 4: Single Key (Hold) Mode

This command configures the button to act like the 'a' key on a keyboard.

**Command:**
```bash
sudo ./usbbutton_tool --configure --permanent --mode default --text "a" --released-color 0,255,0 --pressed-color 255,0,0
```

**Expected Effect:**
*   The button's LED should be green.
*   When you press and hold the button, it should act as if you are holding down the 'a' key (e.g., it should type 'aaaaaaaaa...').
*   When you release the button, the 'a' key should also be released.
