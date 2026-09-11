/* SPDX-License-Identifier: MIT */
#include "render.h"

#include "cat_frames.h"

#include <stddef.h>
#include <string.h>

enum {
    OLED_WIDTH = 32,
    OLED_HEIGHT = 128,
    OLED_ROW_BYTES = 4,
    CAT_TOP = 52,
    CAT_HEIGHT = 52,
};

static const uint8_t glyphs[128][7] = {
    [' '] = {0, 0, 0, 0, 0, 0, 0},
    ['%'] = {25, 26, 2, 4, 8, 11, 19},
    ['-'] = {0, 0, 0, 31, 0, 0, 0},
    ['/'] = {1, 2, 2, 4, 8, 8, 16},
    ['0'] = {14, 17, 19, 21, 25, 17, 14},
    ['1'] = {4, 12, 4, 4, 4, 4, 14},
    ['2'] = {14, 17, 1, 2, 4, 8, 31},
    ['3'] = {30, 1, 1, 14, 1, 1, 30},
    ['4'] = {2, 6, 10, 18, 31, 2, 2},
    ['5'] = {31, 16, 16, 30, 1, 1, 30},
    ['6'] = {14, 16, 16, 30, 17, 17, 14},
    ['7'] = {31, 1, 2, 4, 8, 8, 8},
    ['8'] = {14, 17, 17, 14, 17, 17, 14},
    ['9'] = {14, 17, 17, 15, 1, 1, 14},
    ['A'] = {14, 17, 17, 31, 17, 17, 17},
    ['B'] = {30, 17, 17, 30, 17, 17, 30},
    ['C'] = {14, 17, 16, 16, 16, 17, 14},
    ['D'] = {30, 17, 17, 17, 17, 17, 30},
    ['E'] = {31, 16, 16, 30, 16, 16, 31},
    ['F'] = {31, 16, 16, 30, 16, 16, 16},
    ['G'] = {14, 17, 16, 23, 17, 17, 15},
    ['H'] = {17, 17, 17, 31, 17, 17, 17},
    ['I'] = {14, 4, 4, 4, 4, 4, 14},
    ['J'] = {7, 2, 2, 2, 2, 18, 12},
    ['K'] = {17, 18, 20, 24, 20, 18, 17},
    ['L'] = {16, 16, 16, 16, 16, 16, 31},
    ['M'] = {17, 27, 21, 21, 17, 17, 17},
    ['N'] = {17, 25, 21, 19, 17, 17, 17},
    ['O'] = {14, 17, 17, 17, 17, 17, 14},
    ['P'] = {30, 17, 17, 30, 16, 16, 16},
    ['Q'] = {14, 17, 17, 17, 21, 18, 13},
    ['R'] = {30, 17, 17, 30, 20, 18, 17},
    ['S'] = {15, 16, 16, 14, 1, 1, 30},
    ['T'] = {31, 4, 4, 4, 4, 4, 4},
    ['U'] = {17, 17, 17, 17, 17, 17, 14},
    ['V'] = {17, 17, 17, 17, 17, 10, 4},
    ['W'] = {17, 17, 17, 21, 21, 21, 10},
    ['X'] = {17, 17, 10, 4, 10, 17, 17},
    ['Y'] = {17, 17, 10, 4, 4, 4, 4},
    ['Z'] = {31, 1, 2, 4, 8, 16, 31},
    ['_'] = {0, 0, 0, 0, 0, 0, 31},
};

static const uint8_t small_glyphs[128][5] = {
    [' '] = {0, 0, 0, 0, 0},
    ['-'] = {0, 0, 7, 0, 0},
    ['/'] = {1, 1, 2, 4, 4},
    ['?'] = {6, 1, 2, 0, 2},
    ['0'] = {7, 5, 5, 5, 7},
    ['1'] = {2, 6, 2, 2, 7},
    ['2'] = {7, 1, 7, 4, 7},
    ['3'] = {7, 1, 7, 1, 7},
    ['4'] = {5, 5, 7, 1, 1},
    ['5'] = {7, 4, 7, 1, 7},
    ['6'] = {7, 4, 7, 5, 7},
    ['7'] = {7, 1, 2, 2, 2},
    ['8'] = {7, 5, 7, 5, 7},
    ['9'] = {7, 5, 7, 1, 7},
    ['A'] = {2, 5, 7, 5, 5},
    ['B'] = {6, 5, 6, 5, 6},
    ['C'] = {3, 4, 4, 4, 3},
    ['D'] = {6, 5, 5, 5, 6},
    ['E'] = {7, 4, 6, 4, 7},
    ['F'] = {7, 4, 6, 4, 4},
    ['G'] = {3, 4, 5, 5, 3},
    ['H'] = {5, 5, 7, 5, 5},
    ['I'] = {7, 2, 2, 2, 7},
    ['J'] = {1, 1, 1, 5, 2},
    ['K'] = {5, 5, 6, 5, 5},
    ['L'] = {4, 4, 4, 4, 7},
    ['M'] = {5, 7, 7, 5, 5},
    ['N'] = {5, 7, 7, 7, 5},
    ['O'] = {7, 5, 5, 5, 7},
    ['P'] = {6, 5, 6, 4, 4},
    ['Q'] = {7, 5, 5, 7, 3},
    ['R'] = {6, 5, 6, 5, 5},
    ['S'] = {7, 4, 7, 1, 7},
    ['T'] = {7, 2, 2, 2, 2},
    ['U'] = {5, 5, 5, 5, 7},
    ['V'] = {5, 5, 5, 5, 2},
    ['W'] = {5, 5, 7, 7, 5},
    ['X'] = {5, 5, 2, 5, 5},
    ['Y'] = {5, 5, 2, 2, 2},
    ['Z'] = {7, 1, 2, 4, 7},
    ['_'] = {0, 0, 0, 0, 7},
};

static void put_pixel(uint8_t pixels[512], int x, int y) {
    if (x >= 0 && x < OLED_WIDTH && y >= 0 && y < OLED_HEIGHT) {
        pixels[y * OLED_ROW_BYTES + x / 8] |= (uint8_t)(0x80u >> (x % 8));
    }
}

static void horizontal_line(uint8_t pixels[512], int x1, int x2, int y) {
    for (int x = x1; x <= x2; x++) {
        put_pixel(pixels, x, y);
    }
}

static size_t text_length(const char *text) {
    size_t length = 0;
    while (text[length] != '\0') {
        length++;
    }
    return length;
}

static void draw_text(uint8_t pixels[512], int x, int y, const char *text, bool small,
                      int scale) {
    const int width = small ? 3 : 5;
    const int height = small ? 5 : 7;

    while (*text != '\0') {
        unsigned char character = (unsigned char)*text++;
        const uint8_t *rows = small ? small_glyphs[character] : glyphs[character];
        for (int dy = 0; dy < height; dy++) {
            for (int dx = 0; dx < width; dx++) {
                if ((rows[dy] & (1u << (width - 1 - dx))) != 0) {
                    for (int sy = 0; sy < scale; sy++) {
                        for (int sx = 0; sx < scale; sx++) {
                            put_pixel(pixels, x + dx * scale + sx, y + dy * scale + sy);
                        }
                    }
                }
            }
        }
        x += (width + 1) * scale;
    }
}

static void draw_centered(uint8_t pixels[512], int y, const char *text, bool small,
                          int scale) {
    int advance = small ? 4 : 6;
    int width = ((advance * (int)text_length(text)) - 1) * scale;
    draw_text(pixels, (OLED_WIDTH - width) / 2, y, text, small, scale);
}

static char *append_unsigned(char *destination, unsigned value, unsigned minimum_digits) {
    char reversed[3];
    unsigned digits = 0;
    do {
        reversed[digits++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value != 0u);
    while (digits < minimum_digits) {
        reversed[digits++] = '0';
    }
    while (digits != 0u) {
        *destination++ = reversed[--digits];
    }
    return destination;
}

static void draw_battery(uint8_t pixels[512], uint8_t percent) {
    char label[5];
    char *end = append_unsigned(label, percent, 1);
    *end++ = '%';
    *end = '\0';
    int x = (OLED_WIDTH - (8 + 6 * (int)text_length(label) - 1)) / 2;

    horizontal_line(pixels, x, x + 4, 2);
    horizontal_line(pixels, x, x + 4, 6);
    for (int y = 3; y <= 5; y++) {
        put_pixel(pixels, x, y);
        put_pixel(pixels, x + 4, y);
    }
    put_pixel(pixels, x + 5, 4);

    unsigned bounded_percent = percent > 100 ? 100 : percent;
    unsigned filled = (bounded_percent * 3u + 50u) / 100u;
    for (unsigned dx = 1; dx <= filled; dx++) {
        for (int y = 3; y <= 5; y++) {
            put_pixel(pixels, x + (int)dx, y);
        }
    }
    draw_text(pixels, x + 8, 1, label, false, 1);
}

static char ascii_upper(char value) {
    if (value >= 'a' && value <= 'z') {
        return (char)(value - 'a' + 'A');
    }
    return value;
}

static bool small_character_supported(unsigned char character) {
    return (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9') || character == '-' ||
           character == '/' || character == '?';
}

static void layer_lines(const char *name, char lines[2][9]) {
    size_t line = 0;
    size_t column = 0;
    size_t consumed = 0;

    memset(lines, 0, 18);
    if (name == NULL || name[0] == '\0') {
        memcpy(lines[0], "UNKNOWN", 8);
        return;
    }

    while (*name != '\0' && consumed++ < 16) {
        unsigned char character = (unsigned char)*name++;
        if (character == '_' && line == 0 && column != 0) {
            line = 1;
            column = 0;
            continue;
        }
        if (column == 8) {
            if (line == 1) {
                break;
            }
            line = 1;
            column = 0;
        }
        character = (unsigned char)ascii_upper((char)character);
        lines[line][column++] = small_character_supported(character) ? (char)character : '?';
    }
}

static void render_central(uint8_t pixels[512], const struct oled_view *view) {
    char layer[5] = {'L'};
    char profile[8];
    char *end;

    draw_battery(pixels, view->battery_percent);
    horizontal_line(pixels, 2, 29, 12);
    horizontal_line(pixels, 2, 29, 56);
    horizontal_line(pixels, 2, 29, 90);

    end = append_unsigned(layer + 1, view->layer_index, 1);
    *end = '\0';
    draw_centered(pixels, 22, layer, false, 2);

    char lines[2][9];
    layer_lines(view->layer_name, lines);
    draw_centered(pixels, 40, lines[0], true, 1);
    if (lines[1][0] != '\0') {
        draw_centered(pixels, 47, lines[1], true, 1);
    }

    draw_centered(pixels, 65, "OUT", true, 1);
    if (view->output == OLED_OUTPUT_USB) {
        draw_centered(pixels, 78, "USB", false, 1);
    } else if (view->output == OLED_OUTPUT_BT) {
        draw_centered(pixels, 78, "BT", false, 1);
    } else {
        draw_centered(pixels, 78, "--", false, 1);
    }

    draw_centered(pixels, 98, "PROFILE", true, 1);
    end = append_unsigned(profile, (unsigned)view->profile_index + 1u, 1);
    *end++ = '/';
    end = append_unsigned(end, view->profile_count, 1);
    *end = '\0';
    draw_centered(pixels, 108, profile, false, 1);
    draw_centered(pixels, 120, view->profile_connected ? "OK" : "OFF", false, 1);
}

static void render_peripheral(uint8_t pixels[512], const struct oled_view *view) {
    char wpm[4];

    draw_battery(pixels, view->battery_percent);
    horizontal_line(pixels, 2, 29, 12);
    draw_centered(pixels, 19, "WPM", false, 1);
    if (view->wpm_available) {
        char *end = append_unsigned(wpm, view->wpm, 3);
        *end = '\0';
        draw_centered(pixels, 31, wpm, true, 2);
    } else {
        draw_centered(pixels, 31, "--", true, 2);
    }

    const uint8_t *cat = cat_frames[view->animation_frame % 8u];
    for (int row = 0; row < CAT_HEIGHT; row++) {
        memcpy(pixels + (CAT_TOP + row) * OLED_ROW_BYTES,
               cat + row * OLED_ROW_BYTES, OLED_ROW_BYTES);
    }

    draw_centered(pixels, 114, "SPLIT", true, 1);
    draw_centered(pixels, 121, view->split_connected ? "OK" : "LOST", false, 1);
}

void oled_render(uint8_t pixels[512], const struct oled_view *view) {
    memset(pixels, 0, OLED_HEIGHT * OLED_ROW_BYTES);
    if (view->central) {
        render_central(pixels, view);
    } else {
        render_peripheral(pixels, view);
    }
}
