#define DT_DRV_COMPAT zmk_behavior_oled_wpm

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include "wpm_split.h"
#include "wpm_state.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/wpm.h>
#include <zmk/split/central.h>
#include <zmk/split/transport/central.h>

static void send_wpm(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(wpm_work, send_wpm);

static bool right_is_connected(void) {
    /* Corne has one peripheral, source 0. Query registered transports without
     * replacing their status callbacks or filling the BLE queue while offline. */
    STRUCT_SECTION_FOREACH(zmk_split_transport_central, transport) {
        uint8_t sources[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
        if (!transport->api->get_available_source_ids) {
            continue;
        }
        int count = transport->api->get_available_source_ids(sources);
        for (int i = 0; i < count; i++) {
            if (sources[i] == 0) {
                return true;
            }
        }
    }
    return false;
}

static void send_wpm(struct k_work *work) {
    ARG_UNUSED(work);
    if (right_is_connected()) {
        struct zmk_behavior_binding binding = {
            .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(oled_wpm)),
            .param1 = CLAMP(zmk_wpm_get_state(), 0, UINT8_MAX),
        };
        struct zmk_behavior_binding_event event = {.timestamp = k_uptime_get()};
        int err = zmk_split_central_invoke_behavior(0, &binding, event, true);
        if (err < 0) {
            LOG_DBG("OLED WPM send failed: %d; retry next second", err);
        }
    }
    /* Repeated values, including idle zero, are heartbeats for the receiver. */
    k_work_reschedule(&wpm_work, K_SECONDS(1));
}

static int init_wpm(void) {
    k_work_schedule(&wpm_work, K_SECONDS(1));
    return 0;
}
SYS_INIT(init_wpm, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

bool oled_split_wpm_read(uint8_t *wpm) {
    ARG_UNUSED(wpm);
    return false;
}

static int receive_wpm(struct zmk_behavior_binding *binding,
                       struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return 0;
}
#else
#include <zmk/event_manager.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/split/bluetooth/peripheral.h>

static struct oled_wpm_state state;
static struct k_spinlock state_lock;

static int connection_changed(const zmk_event_t *eh) {
    const struct zmk_split_peripheral_status_changed *ev =
        as_zmk_split_peripheral_status_changed(eh);
    if (ev) {
        k_spinlock_key_t key = k_spin_lock(&state_lock);
        oled_wpm_set_connected(&state, ev->connected);
        k_spin_unlock(&state_lock, key);
    }
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(oled_wpm_connection, connection_changed);
ZMK_SUBSCRIPTION(oled_wpm_connection, zmk_split_peripheral_status_changed);

bool oled_split_wpm_read(uint8_t *wpm) {
    int64_t now = k_uptime_get();
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    bool available = oled_wpm_read(&state, now, wpm);
    k_spin_unlock(&state_lock, key);
    return available;
}

static int receive_wpm(struct zmk_behavior_binding *binding,
                       struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);
    if (binding->param1 > UINT8_MAX) {
        return -EINVAL;
    }
    int64_t now = k_uptime_get();
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    oled_wpm_receive(&state, binding->param1, now);
    k_spin_unlock(&state_lock, key);
    return 0;
}
#endif

static const struct behavior_driver_api wpm_api = {
    .binding_pressed = receive_wpm,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &wpm_api);
