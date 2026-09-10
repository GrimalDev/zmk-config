# Corne portrait OLEDs

Custom 32x128 monochrome layout for the Corne's native 128x32 SSD1306.
The local `corne_oled` shield enables both screens. Firmware uses current ZMK
and LVGL 9; only the Cat artwork is adapted from `zmk-nice-oled`.

| Left (central) | Right (peripheral) |
| --- | --- |
| Local battery percentage | Local battery percentage |
| Active layer index and name | Whole-keyboard WPM |
| Selected USB/BT output | Centered, upward Nyan Cat |
| Active Bluetooth profile out of five, OK/OFF | Split link OK/LOST |

Names come from layer display names, falling back to keymap node names.
Profile numbering is one-based on screen; `BT_SEL` remains zero-based.
The profile's connection status is independent of USB output.

ZMK computes WPM on the central half. Once per second, `wpm_split.c` sends the
value through ZMK's existing split behavior transport to the private `oled_wpm`
behavior. It only updates display state; it emits no key events and needs no
keymap binding. Samples expire after three seconds, and disconnect clears them.
The right screen shows `--` until a fresh sample arrives; idle zero shows `000`.
This transport assumes one Corne peripheral (source 0).

Rendering runs on ZMK's dedicated display queue. Each frame is rotated clockwise
into the SSD1306 buffer without scaling. Default display blanking on idle remains
enabled. The host tests cover WPM freshness and exact portrait pixels; hardware
is needed to verify physical orientation, battery readings and radio delivery.

## Build and flash

Push the configuration branch, then download the `firmware` artifact from its
successful `build.yml` Actions run. Flash **both** `corne-left.uf2` and
`corne-right.uf2`, each to its matching half: connect that half by USB, enter its
UF2 bootloader (usually double-tap reset), then copy the file to the bootloader
drive. Wait for the drive to eject before unplugging.

After flashing, check layer changes, all five profiles and USB/BT selection on
the left. Type on either half and check the right WPM; disconnect the left to
check `LOST`/`--`, then reconnect and check recovery. If both screens appear
upside-down on the physical keyboard, the software rotation in `screen.c` is
the single place to reverse their orientation.
