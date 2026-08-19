# USB Button tool examples

Build in the Nix development shell:

```bash
nix develop --command make check
```

List matching HID interfaces:

```bash
sudo ./build/usbbutton_tool --list
```

Back up and decode the current 64-byte configuration:

```bash
sudo ./build/usbbutton_tool --read --output usbbutton-backup.ubn
```

Configure Ctrl+W while preserving the current colors and reserved settings:

```bash
sudo ./build/usbbutton_tool \
  --configure \
  --input usbbutton-backup.ubn \
  --permanent \
  --mode extended \
  --primary 'ctrl+w' \
  --secondary '' \
  --output ctrl-w.ubn \
  --verbose
```

The tool writes `70 1a 00 00 00 00` into the first primary row. `0x70` is the
USB Button firmware's private Left Ctrl value; `0x1a` is the standard HID usage
for W. The tool reads the configuration back and verifies it by default.

Rows are separated by semicolons. This opens the Run dialog in one row and
types `www.us` after the GUI modifier is released at the row boundary:

```bash
--primary 'win+r;w,w,w,period,u,s'
```

Generate a configuration without accessing hardware:

```bash
./build/usbbutton_tool \
  --configure --dry-run --permanent --mode extended \
  --primary 'ctrl+w' --output ctrl-w.ubn
```
