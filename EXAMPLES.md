# USB Button Tool Examples

This file provides a set of example commands for the `usbbutton_tool` and describes the expected behavior for each command. Please run these commands and report back which ones work as expected and which ones do not.

**Note:** You may need to run these commands with `sudo` depending on your system's USB device permissions.

---

### Example 1: Default Mode (Send all keys)

This command configures the button to type "hello" when pressed.

**Command:**
```bash
./usbbutton_tool --configure --permanent --mode default --text "hello" --released-color 0,255,0 --pressed-color 255,0,0
```

**Expected Effect:**
*   The button's LED should be green.
*   When you press the button, it should type "hello".

---

### Example 2: Alternate Mode

This command configures the button to type "first" on the first press, and "second" on the second press.

**Command:**
```bash
./usbbutton_tool --configure --permanent --mode alternate --text1 "first" --text2 "second" --released-color 0,255,0 --pressed-color 255,0,0
```

**Expected Effect:**
*   The button's LED should be green.
*   On the first press, it should type "first".
*   On the second press, it should type "second".

---

### Example 3: Extended Mode (Multimedia Keys)

This command configures the button to act as a set of multimedia keys. This tests the new hypothesis about extended mode.

**Command:**
```bash
./usbbutton_tool --configure --permanent --mode extended --keys "volume_up,volume_down,mute" --released-color 0,255,0 --pressed-color 255,0,0
```

**Expected Effect:**
*   The button's LED should be green.
*   The first press should increase the system volume.
*   The second press should decrease the system volume.
*   The third press should mute/unmute the system volume.
*   Subsequent presses should cycle through these three actions.

---

### Example 4: Single Key (Hold) Mode

This command configures the button to act like the 'a' key on a keyboard.

**Command:**
```bash
./usbbutton_tool --configure --permanent --mode default --text "a" --released-color 0,255,0 --pressed-color 255,0,0
```

**Expected Effect:**
*   The button's LED should be green.
*   When you press and hold the button, it should act as if you are holding down the 'a' key (e.g., it should type 'aaaaaaaaa...').
*   When you release the button, the 'a' key should also be released.

---

### Example 5: Get Button State

This command queries the button for its current physical state.

**Command (when NOT pressing the button):**
```bash
./usbbutton_tool --get-state
```

**Expected Effect:**
The tool should print: `Button state: Released`

---

### A Note on Control Sequences (e.g., Ctrl+R)

Support for sending modifier keys like `Ctrl`, `Shift`, or `Alt` is not yet implemented, as the protocol for this is not documented in the available materials.
