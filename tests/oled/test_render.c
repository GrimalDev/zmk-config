/* SPDX-License-Identifier: MIT */
#include "render.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { OLED_BUFFER_SIZE = 512, GUARD_SIZE = 16 };

static unsigned checks;
static unsigned failures;

static void expect_golden(const char *name, const struct oled_view *view,
                          const char *fixture_path) {
    uint8_t actual[OLED_BUFFER_SIZE];
    uint8_t expected[OLED_BUFFER_SIZE];
    FILE *fixture = fopen(fixture_path, "rb");

    checks++;
    if (fixture == NULL || fread(expected, 1, sizeof(expected), fixture) != sizeof(expected) ||
        fgetc(fixture) != EOF) {
        failures++;
        fprintf(stderr, "FAIL %s: cannot read 512-byte fixture %s\n", name, fixture_path);
        if (fixture != NULL) {
            fclose(fixture);
        }
        return;
    }
    fclose(fixture);

    oled_render(actual, view);
    if (memcmp(actual, expected, sizeof(actual)) != 0) {
        size_t offset = 0;
        while (offset < sizeof(actual) && actual[offset] == expected[offset]) {
            offset++;
        }
        failures++;
        fprintf(stderr, "FAIL %s: first mismatch at byte %zu: got 0x%02x, expected 0x%02x\n",
                name, offset, actual[offset], expected[offset]);
    }
}

static void expect_bounds_and_unknown_layer(void) {
    uint8_t guarded[OLED_BUFFER_SIZE + 2 * GUARD_SIZE];
    struct oled_view view = {
        .central = true,
        .battery_percent = 255,
        .layer_index = 255,
        .layer_name = "an-unknown/name that is much too long",
        .output = OLED_OUTPUT_NONE,
        .profile_index = 255,
        .profile_count = 0,
    };

    memset(guarded, 0xa5, sizeof(guarded));
    oled_render(guarded + GUARD_SIZE, &view);

    checks++;
    for (size_t index = 0; index < GUARD_SIZE; index++) {
        if (guarded[index] != 0xa5 || guarded[GUARD_SIZE + OLED_BUFFER_SIZE + index] != 0xa5) {
            failures++;
            fprintf(stderr, "FAIL bounds: guard byte %zu changed\n", index);
            return;
        }
    }

    checks++;
    bool has_layer_pixels = false;
    for (size_t row = 39; row < 55; row++) {
        for (size_t column = 0; column < 4; column++) {
            has_layer_pixels |= guarded[GUARD_SIZE + row * 4 + column] != 0;
        }
    }
    if (!has_layer_pixels) {
        failures++;
        fprintf(stderr, "FAIL unknown layer: fallback label is blank\n");
    }
}

static void expect_custom_layer_name(void) {
    uint8_t lowercase[OLED_BUFFER_SIZE];
    uint8_t uppercase[OLED_BUFFER_SIZE];
    uint8_t missing[OLED_BUFFER_SIZE];
    struct oled_view view = {
        .central = true,
        .battery_percent = 50,
        .layer_index = 7,
        .layer_name = "nav_mouse",
        .output = OLED_OUTPUT_NONE,
        .profile_count = 5,
    };

    oled_render(lowercase, &view);
    view.layer_name = "NAV_MOUSE";
    oled_render(uppercase, &view);
    view.layer_name = NULL;
    oled_render(missing, &view);

    checks++;
    if (memcmp(lowercase, uppercase, sizeof(lowercase)) != 0 ||
        memcmp(lowercase, missing, sizeof(lowercase)) == 0) {
        failures++;
        fprintf(stderr, "FAIL custom layer: name was not rendered case-insensitively\n");
    }
}

int main(void) {
    const struct oled_view central_bt = {
        .central = true,
        .battery_percent = 87,
        .layer_index = 0,
        .layer_name = "Qwerty",
        .output = OLED_OUTPUT_BT,
        .profile_index = 1,
        .profile_count = 5,
        .profile_connected = true,
    };
    const struct oled_view central_usb = {
        .central = true,
        .battery_percent = 87,
        .layer_index = 3,
        .layer_name = "media_ext",
        .output = OLED_OUTPUT_USB,
        .profile_index = 4,
        .profile_count = 5,
        .profile_connected = false,
    };
    const struct oled_view central_number = {
        .central = true,
        .battery_percent = 87,
        .layer_index = 1,
        .layer_name = "number",
        .output = OLED_OUTPUT_BT,
        .profile_index = 1,
        .profile_count = 5,
        .profile_connected = true,
    };
    const struct oled_view central_system = {
        .central = true,
        .battery_percent = 87,
        .layer_index = 2,
        .layer_name = "SYSTEM",
        .output = OLED_OUTPUT_BT,
        .profile_index = 1,
        .profile_count = 5,
        .profile_connected = true,
    };
    const struct oled_view peripheral_connected = {
        .central = false,
        .battery_percent = 82,
        .split_connected = true,
        .wpm_available = true,
        .wpm = 38,
        .animation_frame = 0,
    };
    const struct oled_view peripheral_lost = {
        .central = false,
        .battery_percent = 82,
        .split_connected = false,
        .wpm_available = false,
        .animation_frame = 0,
    };

    expect_golden("central Bluetooth", &central_bt,
                  "tests/oled/fixtures/central-bt-qwerty.bin");
    expect_golden("central number layer", &central_number,
                  "tests/oled/fixtures/central-bt-number.bin");
    expect_golden("central system layer", &central_system,
                  "tests/oled/fixtures/central-bt-system.bin");
    expect_golden("central USB", &central_usb,
                  "tests/oled/fixtures/central-usb-media-ext.bin");
    for (uint8_t frame = 0; frame < 8; frame++) {
        char name[32];
        char path[48];
        struct oled_view animated = peripheral_connected;
        animated.animation_frame = frame;
        snprintf(name, sizeof(name), "peripheral frame %u", frame);
        snprintf(path, sizeof(path), "tests/oled/fixtures/peripheral-frame-%u.bin", frame);
        expect_golden(name, &animated, path);
    }
    expect_golden("peripheral lost", &peripheral_lost,
                  "tests/oled/fixtures/peripheral-lost.bin");
    expect_bounds_and_unknown_layer();
    expect_custom_layer_name();

    printf("%u checks, %u failures\n", checks, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
