/* SPDX-License-Identifier: MIT */
#pragma once

#include <stdbool.h>
#include <stdint.h>

enum oled_output {
    OLED_OUTPUT_USB,
    OLED_OUTPUT_BT,
    OLED_OUTPUT_NONE,
};

struct oled_view {
    bool central;
    uint8_t battery_percent;
    uint8_t layer_index;
    const char *layer_name;
    enum oled_output output;
    uint8_t profile_index;
    uint8_t profile_count;
    bool profile_connected;
    bool split_connected;
    bool wpm_available;
    uint8_t wpm;
    uint8_t animation_frame;
};

/* 32 x 128, four bytes per row, most-significant pixel bit first. */
void oled_render(uint8_t pixels[512], const struct oled_view *view);
