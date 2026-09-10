#include <string.h>
#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/battery.h>
#include <zmk/display/status_screen.h>
#include "render.h"
#include "wpm_split.h"

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#define OLED_LAYER_NODE_NAME(node) DT_NODE_FULL_NAME(node)
static const char *const layer_node_names[] = {
    ZMK_KEYMAP_LAYERS_FOREACH_SEP(OLED_LAYER_NODE_NAME, (,))};
#else
#include <zmk/split/bluetooth/peripheral.h>
#endif

static uint8_t portrait[512];
/* Zephyr 4.1's LVGL mono conversion reverses I1 luminance on the SSD1306.
 * Compensate here: index 0 must be physically dark, index 1 physically lit.
 * Revisit when upgrading that conversion; portrait assets keep normal polarity. */
static uint8_t image_data[520] = {255, 255, 255, 255, 0, 0, 0, 255};
static const lv_image_dsc_t image_descriptor = {
    .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_I1,
               .w = 128, .h = 32, .stride = 16},
    .data_size = sizeof(image_data),
    .data = image_data,
};
static lv_obj_t *image;

static void refresh(lv_timer_t *timer) {
    ARG_UNUSED(timer);
    struct oled_view view = {
        .battery_percent = zmk_battery_state_of_charge(),
        .animation_frame = (k_uptime_get() / 120) % 8,
    };
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    view.central = true;
    view.layer_index = zmk_keymap_highest_layer_active();
    zmk_keymap_layer_id_t layer_id = zmk_keymap_layer_index_to_id(view.layer_index);
    view.layer_name = zmk_keymap_layer_name(layer_id);
    /* Keymap-editor layers have node names but no display-name property. */
    if ((!view.layer_name || !view.layer_name[0]) && layer_id < ARRAY_SIZE(layer_node_names)) {
        view.layer_name = layer_node_names[layer_id];
    }
    enum zmk_transport transport = zmk_endpoint_get_selected().transport;
    if (transport == ZMK_TRANSPORT_NONE) {
        transport = zmk_endpoint_get_preferred_transport();
    }
    view.output = transport == ZMK_TRANSPORT_USB ? OLED_OUTPUT_USB : OLED_OUTPUT_BT;
    view.profile_index = zmk_ble_active_profile_index();
    view.profile_count = ZMK_BLE_PROFILE_COUNT;
    view.profile_connected = zmk_ble_active_profile_is_connected();
#else
    view.split_connected = zmk_split_bt_peripheral_is_connected();
    view.wpm_available = oled_split_wpm_read(&view.wpm) && view.split_connected;
#endif
    oled_render(portrait, &view);
    memset(image_data + 8, 0, 512);
    /* Clockwise software rotation maps the portrait layout to the SSD1306's
     * native landscape buffer; no LVGL interpolation changes the tiny glyphs. */
    for (unsigned y = 0; y < 128; y++) {
        for (unsigned x = 0; x < 32; x++) {
            if (portrait[y * 4 + x / 8] & (0x80 >> (x % 8))) {
                unsigned dx = 127 - y;
                unsigned dy = x;
                image_data[8 + dy * 16 + dx / 8] |= 0x80 >> (dx % 8);
            }
        }
    }
    lv_image_cache_drop(&image_descriptor);
    lv_obj_invalidate(image);
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    /* Match the compensated palette's background, including uncovered pixels. */
    lv_obj_set_style_bg_color(screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    image = lv_image_create(screen);
    lv_image_set_src(image, &image_descriptor);
    lv_obj_set_pos(image, 0, 0);
    refresh(NULL);
    lv_timer_create(refresh, 120, NULL);
    return screen;
}
