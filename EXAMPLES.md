# USB Button Tool Examples

This file provides a set of example commands for the `usbbutton_tool` and describes the expected behavior for each command. Please run these commands and report back which ones work as expected and which ones do not.

**Note:** You may need to run these commands with `sudo` depending on your system's USB device permissions.

---

### Example 1: Configure Temporary Behavior

This command configures the button to have a green released color, a red pressed color, and to type "test" when pressed. This configuration is temporary and will be lost if the button is unplugged.

**Command:**
```bash
./usbbutton_tool --configure --released-color 0,255,0 --pressed-color 255,0,0 --text "test"
```

**Expected Effect:**
1.  **Initial State:** After running the command, the button's LED should immediately turn green.
2.  **Press and Hold:** When you press and hold the button, the LED should turn red.
3.  **Release:** When you release the button, the LED should turn back to green.
4.  **Typing:** When you press and release the button, it should type the word "test" into any active text editor or terminal.

---

### Example 2: Configure Permanent Behavior

This command does the same as Example 1, but this time the configuration is stored permanently in the button's memory. We'll use different colors and text to distinguish it.

**Command:**
```bash
./usbbutton_tool --configure --permanent --released-color 0,0,255 --pressed-color 255,255,0 --text "permanent"
```

**Expected Effect:**
1.  **Initial State:** After running the command, the button's LED should immediately turn blue.
2.  **Press and Hold:** When you press and hold the button, the LED should turn yellow (red + green).
3.  **Release:** When you release the button, the LED should turn back to blue.
4.  **Typing:** When you press and release the button, it should type the word "permanent".
5.  **Persistence:** If you unplug the button and plug it back in, the behavior described in the steps above should persist without needing to run the command again.

---

### Example 3: Get Button State

This command queries the button for its current physical state (pressed or released).

**Command (when NOT pressing the button):**
```bash
./usbbutton_tool --get-state
```

**Expected Effect:**
The tool should print the following output to your terminal:
```
Button state: Released
```

**Command (while holding the button down):**
```bash
./usbbutton_tool --get-state
```

**Expected Effect:**
The tool should print the following output to your terminal:
```
Button state: Pressed
```

Please let me know the results of these tests, and we can continue debugging from there.
