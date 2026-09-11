/* SPDX-License-Identifier: MIT */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Zero initialization represents a disconnected display with no sample.
 * Callers serialize access and supply monotonic uptime in milliseconds.
 */
struct oled_wpm_state {
    bool connected;
    bool has_sample;
    uint8_t wpm;
    int64_t received_at_ms;
};

void oled_wpm_set_connected(struct oled_wpm_state *state, bool connected);
void oled_wpm_receive(struct oled_wpm_state *state, uint8_t wpm, int64_t now_ms);

/* False means the display must show --. Samples expire at age 3000 ms.
 * A false result leaves the output value unchanged.
 */
bool oled_wpm_read(const struct oled_wpm_state *state, int64_t now_ms, uint8_t *wpm);
