# USB Button Tool Examples

This file provides a set of example commands for the `usbbutton_tool` and describes the expected behavior for each command. Please run these commands and report back which ones work as expected and which ones do not.

**Note:** You may need to run these commands with `sudo` depending on your system's USB device permissions.

---

### Example 1: Default Mode (Send all keys)

This command configures the button to type "hello world" when pressed.

**Command:**
```bash
./usbbutton_tool --configure --permanent --mode default --text "hello world" --released-color 0,255,0 --pressed-color 255,0,0
```

**Expected Effect:**
1.  **Initial State:** The button's LED should be green.
2.  **Press and Hold:** The LED should turn red.
3.  **Typing:** When you press and release the button, it should type "hello world".

---

### Example 2: Alternate Mode

This command configures the button to type "first" on the first press, and "second" on the second press, and so on.

**Command:**
```bash
./usbbutton_tool --configure --permanent --mode alternate --text1 "first" --text2 "second" --released-color 0,255,0 --pressed-color 255,0,0
```

**Expected Effect:**
1.  **Initial State:** The button's LED should be green.
2.  **First Press:** When you press the button, it should type "first".
3.  **Second Press:** When you press the button again, it should type "second".
4.  **Third Press:** It should type "first" again.

---

### Example 3: Extended Mode

This command configures the button to type "short" on a short press, and "long" on a long press.

**Command:**
```bash
./usbbutton_tool --configure --permanent --mode extended --text1 "short" --text2 "long" --released-color 0,255,0 --pressed-color 255,0,0
```

**Expected Effect:**
1.  **Initial State:** The button's LED should be green.
2.  **Short Press:** A quick press and release should type "short".
3.  **Long Press:** Pressing and holding the button for a second or two before releasing should type "long".

---

### Example 4: Single Key (Hold) Mode

This command configures the button to act like a single keyboard key ('a').

**Command:**
```bash
./usbbutton_tool --configure --permanent --mode default --text "a" --released-color 0,255,0 --pressed-color 255,0,0
```

**Expected Effect:**
1.  **Initial State:** The button's LED should be green.
2.  **Press and Hold:** The LED should turn red. While you are holding the button down, it should act as if you are holding down the 'a' key on your keyboard (e.g., it should type 'aaaaaaaaa...').
3.  **Release:** When you release the button, the 'a' key should also be released.

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

Support for sending modifier keys like `Ctrl`, `Shift`, or `Alt` is not yet implemented. The device's configuration protocol for this is not documented in the available source code. This feature can be added in the future if more information becomes available.
