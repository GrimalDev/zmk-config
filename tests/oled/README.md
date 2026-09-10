# OLED state tests

Run from the repository root with a C11 compiler. These tests exercise WPM
sample validity with explicit timestamps; they need neither Zephyr nor hardware.

```sh
cc -std=c11 -Wall -Wextra -Werror \
  -Imodules/corne_oled \
  modules/corne_oled/wpm_state.c tests/oled/test_wpm_state.c \
  -o /tmp/corne-oled-wpm-test
/tmp/corne-oled-wpm-test
```

The receiver exposes only samples received while connected and less than three
seconds old. Zero is a valid idle speed. Disconnect clears the sample; reconnect
requires a new one. Repeated values refresh the deadline.

The portrait renderer also runs without Zephyr:

```sh
cc -std=c11 -Wall -Wextra -Werror \
  -Imodules/corne_oled \
  modules/corne_oled/render.c tests/oled/test_render.c \
  -o /tmp/corne-oled-render-test
/tmp/corne-oled-render-test
```

The 512-byte fixtures freeze the approved preview's 32x128 pixels, packed
most-significant bit first. They cover the four current layers, USB/BT profile
states, all eight upward Cat frames and split disconnection. Keep the fixtures
independent of the firmware renderer so a rendering regression cannot silently
rewrite its expected result.

CI runs both suites with AddressSanitizer and UBSan. Hardware checks remain
necessary for physical orientation, Bluetooth WPM delivery and battery readings.
