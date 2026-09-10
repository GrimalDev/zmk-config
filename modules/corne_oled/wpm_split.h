#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Peripheral display snapshot; false means disconnected or stale. */
bool oled_split_wpm_read(uint8_t *wpm);
