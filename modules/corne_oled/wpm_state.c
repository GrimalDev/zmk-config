/* SPDX-License-Identifier: MIT */
#include "wpm_state.h"

enum { WPM_SAMPLE_TTL_MS = 3000 };

void oled_wpm_set_connected(struct oled_wpm_state *state, bool connected) {
    state->connected = connected;
    if (!connected) {
        state->has_sample = false;
    }
}

void oled_wpm_receive(struct oled_wpm_state *state, uint8_t wpm, int64_t now_ms) {
    if (!state->connected) {
        return;
    }

    state->wpm = wpm;
    state->received_at_ms = now_ms;
    state->has_sample = true;
}

bool oled_wpm_read(const struct oled_wpm_state *state, int64_t now_ms, uint8_t *wpm) {
    if (!state->connected || !state->has_sample || now_ms < state->received_at_ms ||
        (uint64_t)now_ms - (uint64_t)state->received_at_ms >= WPM_SAMPLE_TTL_MS) {
        return false;
    }

    *wpm = state->wpm;
    return true;
}
