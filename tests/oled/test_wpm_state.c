/* SPDX-License-Identifier: MIT */
#include "wpm_state.h"

#include <stdio.h>

static unsigned failures;
static unsigned checks;

static void expect_sample(const char *name, const struct oled_wpm_state *state,
                          int64_t now_ms, bool expected_available, uint8_t expected_wpm) {
    uint8_t actual_wpm = 0;
    bool available = oled_wpm_read(state, now_ms, &actual_wpm);
    checks++;
    if (available != expected_available || (available && actual_wpm != expected_wpm)) {
        failures++;
        fprintf(stderr, "FAIL %s: available=%d wpm=%u; expected available=%d wpm=%u\n",
                name, available, actual_wpm, expected_available, expected_wpm);
    }
}

int main(void) {
    struct oled_wpm_state state = {0};
    expect_sample("startup has no sample", &state, 0, false, 0);

    oled_wpm_set_connected(&state, true);
    expect_sample("connection alone is not a speed sample", &state, 1000, false, 0);
    oled_wpm_receive(&state, 42, 1000);
    expect_sample("received speed becomes visible", &state, 1000, true, 42);
    expect_sample("speed remains fresh before deadline", &state, 3999, true, 42);
    expect_sample("speed expires at three seconds", &state, 4000, false, 0);

    oled_wpm_receive(&state, 65, 4000);
    expect_sample("new sample replaces stale speed", &state, 4000, true, 65);
    oled_wpm_receive(&state, 65, 5000);
    expect_sample("same speed still refreshes the deadline", &state, 7999, true, 65);

    oled_wpm_receive(&state, 0, 8000);
    expect_sample("idle zero is visible", &state, 8000, true, 0);
    oled_wpm_set_connected(&state, true);
    expect_sample("repeated connected notification keeps fresh sample", &state, 8100, true, 0);

    oled_wpm_set_connected(&state, false);
    expect_sample("disconnect immediately hides speed", &state, 8101, false, 0);
    oled_wpm_set_connected(&state, true);
    expect_sample("reconnect cannot revive the old sample", &state, 8102, false, 0);
    oled_wpm_receive(&state, 17, 8103);
    expect_sample("first new sample after reconnect becomes visible", &state, 8103, true, 17);

    oled_wpm_set_connected(&state, false);
    oled_wpm_receive(&state, 99, 8200);
    oled_wpm_set_connected(&state, true);
    expect_sample("late update while disconnected cannot survive reconnect", &state, 8201, false, 0);

    oled_wpm_receive(&state, 255, 9000);
    expect_sample("full ZMK uint8 speed range is supported", &state, 9000, true, 255);
    expect_sample("timestamp from the future is unavailable", &state, 8999, false, 0);

    printf("%u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
