// Tower Bloxx application page for the 240x320 AI Passport display.
// The button callback only queues inputs; all LVGL work runs from the LVGL timer.
#include "demo.h"

#include "bsp_battery.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lvgl.h"
#include "tower_model.h"
#include "tower_audio.h"
#include "tower_theme_store.h"
#include "ui_pixel.h"

#include <stdbool.h>
#include <stdint.h>

#define FIELD_X 7
#define FIELD_Y 52
#define FLOOR_SLOTS 3
#define TICK_MS 30
#define INTRO_MS 1800U

// CMake embeds this 240x320 RGB565 cover in Flash. LVGL reads it without
// allocating a full-screen pixel buffer in the ESP32-C3's internal RAM.
extern const uint8_t tower_cover_start[] asm("_binary_tower_cover_rgb565_start");
extern const uint8_t tower_backdrop_start[] asm("_binary_tower_backdrop_rgb565_start");
extern const uint8_t tower_cover_light_start[] asm("_binary_tower_cover_light_rgb565_start");
extern const uint8_t tower_backdrop_light_start[] asm("_binary_tower_backdrop_light_rgb565_start");
static const lv_image_dsc_t TOWER_COVER = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_RGB565,
        .w = 240,
        .h = 320,
        .stride = 480,
    },
    .data_size = 240U * 320U * 2U,
    .data = tower_cover_start,
};
static const lv_image_dsc_t TOWER_BACKDROP = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_RGB565,
        .w = 240,
        .h = 320,
        .stride = 480,
    },
    .data_size = 240U * 320U * 2U,
    .data = tower_backdrop_start,
};
static const lv_image_dsc_t TOWER_COVER_LIGHT = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_RGB565,
        .w = 240,
        .h = 320,
        .stride = 480,
    },
    .data_size = 240U * 320U * 2U,
    .data = tower_cover_light_start,
};
static const lv_image_dsc_t TOWER_BACKDROP_LIGHT = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_RGB565,
        .w = 240,
        .h = 320,
        .stride = 480,
    },
    .data_size = 240U * 320U * 2U,
    .data = tower_backdrop_light_start,
};

static const uint8_t CITY_TYPES[] = { 10, 20, 30, 40 };

// All s_* LVGL objects are owned by s_scr. The LVGL timer owns every mutation.
static tower_model_t s_model;
static lv_obj_t *s_scr;
static lv_obj_t *s_battery_label;
static lv_obj_t *s_playfield;
static lv_obj_t *s_floor_slots[FLOOR_SLOTS];
static lv_obj_t *s_active_floor;
static lv_obj_t *s_crane_pulley;
static lv_obj_t *s_crane_rope;
static lv_obj_t *s_foundation;
static lv_obj_t *s_stats_label;
static lv_obj_t *s_notice_label;
static lv_obj_t *s_menu_cards[3];
static lv_obj_t *s_menu_labels[3];
static lv_obj_t *s_loading_fill;
static lv_obj_t *s_city_cells[TOWER_CITY_CELLS];
static lv_obj_t *s_tier_cards[4];
static lv_obj_t *s_city_hint;
static lv_timer_t *s_timer;
static QueueHandle_t s_input_queue;
static tower_phase_t s_drawn_phase;
static uint64_t s_last_tick_ms;
static uint64_t s_notice_until_ms;
static const char *s_notice_text; // Static feedback string; no heap ownership.
static int s_battery_soc = -2;  // -2 = unread; -1 = unavailable; 0..100 = percent.
static uint8_t s_menu_choice;    // 0 city, 1 quick, 2 theme.
static uint8_t s_city_cursor;    // Selected 0..15 map cell.
static uint8_t s_tier_cursor;    // Selected 0..3 next tower tier.
static bool s_rebuild;
static bool s_intro_active;
static uint64_t s_intro_start_ms;
static tower_theme_t s_theme;
static const tower_palette_t *s_palette;

typedef struct {
    bsp_btn_t button;
    bsp_btn_ev_t event;
} tower_input_t;

static uint64_t now_ms(void)
{
    return (uint64_t)esp_timer_get_time() / 1000ULL;
}

// Construct one low-overhead rectangular pixel-art object; screen owns it.
static lv_obj_t *pixel_block(lv_obj_t *parent, int x, int y, int w, int h,
                             uint32_t fill, uint32_t border, int border_width)
{
    lv_obj_t *object = lv_obj_create(parent);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, w, h);
    lv_obj_set_style_radius(object, 0, 0);
    lv_obj_set_style_pad_all(object, 0, 0);
    lv_obj_set_style_bg_color(object, lv_color_hex(fill), 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(object, lv_color_hex(border), 0);
    lv_obj_set_style_border_width(object, border_width, 0);
    lv_obj_remove_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    return object;
}

// Place a Montserrat label. The firmware baseline intentionally has no CJK font.
static lv_obj_t *text_at(lv_obj_t *parent, const char *text, int x, int y,
                         const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = ui_pixel_label(parent, text, font, color);
    lv_obj_set_pos(label, x, y);
    return label;
}

static lv_obj_t *art_screen(const lv_image_dsc_t *art)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_bg_color(screen, lv_color_hex(s_palette->footer), 0);
    lv_obj_t *cover = lv_image_create(screen);
    lv_image_set_src(cover, art);
    lv_obj_set_pos(cover, 0, 0);
    return screen;
}

static lv_obj_t *cover_screen(void)
{
    return art_screen(s_theme == TOWER_THEME_LIGHT ? &TOWER_COVER_LIGHT : &TOWER_COVER);
}

// The splash is a short branded transition, never a loading operation that can
// stall on storage or networking. Its bar only advances from the LVGL timer.
static void build_splash(void)
{
    s_scr = cover_screen();
    text_at(s_scr, "FOLOTOY PRESENTS", 17, 13,
            &lv_font_montserrat_14, s_palette->on_art);
    text_at(s_scr, "TOWER BLOXX", 17, 35,
            &lv_font_montserrat_20, s_palette->title);
    text_at(s_scr, "BUILDING YOUR CITY", 23, 264,
            &lv_font_montserrat_14, s_palette->on_art);
    pixel_block(s_scr, 21, 290, 198, 11, s_palette->footer, s_palette->on_art, 2);
    s_loading_fill = pixel_block(s_scr, 24, 293, 1, 5,
                                 s_palette->yellow, s_palette->yellow, 0);
    lv_screen_load(s_scr);
}

// Draw the 4x4 facade in one object. Sixteen child objects per floor would
// exhaust the board's fixed 24 KB LVGL pool as the stack moves.
static void draw_floor_windows(lv_event_t *event)
{
    lv_obj_t *floor = lv_event_get_target_obj(event);
    lv_layer_t *layer = lv_event_get_layer(event);
    lv_area_t bounds;
    lv_obj_get_coords(floor, &bounds);
    lv_draw_fill_dsc_t window;
    lv_draw_fill_dsc_init(&window);
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            window.color = lv_color_hex((row + col) % 5 == 0
                                        ? s_palette->window_dark : s_palette->window_bright);
            lv_area_t cell = {
                .x1 = bounds.x1 + 5 + col * 10,
                .y1 = bounds.y1 + 5 + row * 10,
                .x2 = bounds.x1 + 11 + col * 10,
                .y2 = bounds.y1 + 11 + row * 10,
            };
            lv_draw_fill(layer, &window, &cell);
        }
    }
}

// Build one reusable square apartment module.
static lv_obj_t *apartment_floor(lv_obj_t *parent, int x, int y)
{
    lv_obj_t *floor = pixel_block(parent, x, y, TOWER_FLOOR_WIDTH,
                                  TOWER_FLOOR_HEIGHT, s_palette->floors[0], s_palette->ink, 2);
    lv_obj_add_event_cb(floor, draw_floor_windows, LV_EVENT_DRAW_MAIN_END, NULL);
    return floor;
}

// Battery status is read once by the LVGL timer, never by the button task.
static void refresh_battery(void)
{
    if (!s_battery_label) return;
    if (s_battery_soc >= 0) {
        lv_label_set_text_fmt(s_battery_label, "%d%%", s_battery_soc);
    } else {
        lv_label_set_text(s_battery_label, "");
    }
}

static void refresh_menu_choice(void)
{
    lv_label_set_text(s_menu_labels[2], s_theme == TOWER_THEME_LIGHT
                      ? "03   THEME: LIGHT" : "03   THEME: DARK");
    for (int i = 0; i < 3; i++) {
        bool selected = i == s_menu_choice;
        lv_obj_set_style_bg_color(s_menu_cards[i],
                                  lv_color_hex(selected ? s_palette->menu_selected
                                                        : s_palette->menu_bg), 0);
        lv_obj_set_style_border_color(s_menu_cards[i],
                                      lv_color_hex(selected ? s_palette->menu_selected_border
                                                            : s_palette->menu_border), 0);
        lv_obj_set_style_text_color(s_menu_labels[i],
                                    lv_color_hex(selected ? s_palette->menu_selected_text
                                                          : s_palette->menu_text), 0);
    }
}

// The cover art remains visible above three visible physical-key choices.
static void build_menu(void)
{
    text_at(s_scr, "TOWER BLOXX", 16, 13,
            &lv_font_montserrat_20, s_palette->title);
    text_at(s_scr, "STACK THE CITY", 17, 40,
            &lv_font_montserrat_14, s_palette->on_art);
    static const char *choices[] = {
        "01   CITY MODE", "02   QUICK GAME", "03   THEME: DARK"
    };
    for (int i = 0; i < 3; i++) {
        s_menu_cards[i] = pixel_block(s_scr, 17, 205 + i * 37, 206, 31,
                                      s_palette->menu_bg, s_palette->menu_border, 2);
        s_menu_labels[i] = text_at(s_menu_cards[i], choices[i], 12, 7,
                                    &lv_font_montserrat_14, s_palette->menu_text);
    }
    refresh_menu_choice();
}

// Create the tower field once per gameplay entry. Timed movement only repositions
// a fixed set of three floor objects, so LVGL's 24 KB pool is not churned per frame.
static void build_play(void)
{
    text_at(s_scr, "TOWER", 15, 11, &lv_font_montserrat_20, s_palette->title);
    s_playfield = pixel_block(s_scr, FIELD_X, FIELD_Y, 226, TOWER_FIELD_HEIGHT,
                              s_palette->paper, s_palette->ink, 3);
    lv_obj_set_style_bg_opa(s_playfield, LV_OPA_TRANSP, 0);
    pixel_block(s_playfield, 3, 2, 220, 6, s_palette->orange, s_palette->ink, 1);
    s_crane_rope = pixel_block(s_playfield, 0, 11, 2, 8, s_palette->ink, s_palette->ink, 0);
    s_crane_pulley = pixel_block(s_playfield, 0, 5, 13, 9,
                                  s_palette->yellow, s_palette->ink, 2);
    s_foundation = pixel_block(s_playfield, TOWER_CENTER_X - 34,
                                TOWER_BASE_Y, 68, 7,
                                s_palette->muted, s_palette->ink, 2);
    for (int i = 0; i < FLOOR_SLOTS; i++) {
        s_floor_slots[i] = apartment_floor(s_playfield, 0, 0);
        lv_obj_add_flag(s_floor_slots[i], LV_OBJ_FLAG_HIDDEN);
    }
    s_active_floor = apartment_floor(s_playfield, 0, 0);
    s_notice_label = text_at(s_scr, s_notice_text ? s_notice_text : "", 29, 138,
                              &lv_font_montserrat_20, s_palette->ink);
    lv_obj_set_width(s_notice_label, 182);
    lv_obj_set_style_text_align(s_notice_label, LV_TEXT_ALIGN_CENTER, 0);
    pixel_block(s_scr, 0, 286, 240, 34, s_palette->footer, s_palette->orange, 2);
    s_stats_label = text_at(s_scr, "", 11, 290, &lv_font_montserrat_14,
                            s_palette->on_footer);
    text_at(s_scr, "OK DROP       HOLD OK EXIT", 11, 305,
            &lv_font_montserrat_14, s_palette->on_footer);
}

// Resolve the visual floor style from its tower height; no bitmap assets are loaded.
static void position_floor(lv_obj_t *floor, int x, int y, uint8_t style)
{
    lv_obj_set_pos(floor, x - TOWER_FLOOR_WIDTH / 2, y);
    lv_obj_set_style_bg_color(floor, lv_color_hex(s_palette->floors[style % 4]), 0);
    lv_obj_remove_flag(floor, LV_OBJ_FLAG_HIDDEN);
}

// Render model coordinates in the field and scroll older floors off-screen.
// Called only on the LVGL timer, including after queued button inputs.
static void refresh_play(void)
{
    if (!s_active_floor) return;
    int camera = tower_model_camera_offset(&s_model);
    int first = s_model.floor_count > FLOOR_SLOTS
        ? s_model.floor_count - FLOOR_SLOTS : 0;
    for (int slot = 0; slot < FLOOR_SLOTS; slot++) {
        int floor_index = first + slot;
        lv_obj_t *floor = s_floor_slots[slot];
        if (floor_index >= s_model.floor_count) {
            lv_obj_add_flag(floor, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        int y = TOWER_BASE_Y -
                (floor_index + 1) * TOWER_FLOOR_HEIGHT + camera;
        position_floor(floor, s_model.floor_x[floor_index], y,
                       (uint8_t)floor_index);
    }
    if (camera) lv_obj_add_flag(s_foundation, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(s_foundation, LV_OBJ_FLAG_HIDDEN);

    int active_y = (s_model.phase == TOWER_FALLING
                             ? s_model.fall_y : TOWER_ACTIVE_Y);
    position_floor(s_active_floor, s_model.active_x, active_y,
                   s_model.floor_count);
    lv_obj_set_x(s_crane_pulley, s_model.active_x - 6);
    lv_obj_set_x(s_crane_rope, s_model.active_x);
    if (s_model.phase == TOWER_FALLING)
        lv_obj_add_flag(s_crane_rope, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(s_crane_rope, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text_fmt(s_stats_label, "F%02u/%02u  POP%05lu  X%u",
                          (unsigned)s_model.floor_count,
                          (unsigned)(s_model.mode == TOWER_MODE_CITY ? s_model.target : 99),
                          (unsigned long)s_model.population,
                          (unsigned)s_model.misses);
    if (s_notice_until_ms && now_ms() >= s_notice_until_ms) {
        lv_label_set_text(s_notice_label, "");
        s_notice_until_ms = 0;
        s_notice_text = NULL;
    }
}

// Show a compact outcome with a clear single-key next action.
static void build_result(void)
{
    bool complete = s_model.mode == TOWER_MODE_CITY && s_model.pending_type;
    text_at(s_scr, "TOWER BLOXX", 17, 17,
            &lv_font_montserrat_20, s_palette->title);
    pixel_block(s_scr, 19, 82, 210, 173, s_palette->muted, s_palette->muted, 0);
    pixel_block(s_scr, 15, 77, 210, 173, s_palette->paper, s_palette->ink, 3);
    text_at(s_scr, complete ? "TOWER BUILT!" : "GAME OVER", 30, 93,
            &lv_font_montserrat_20, complete ? s_palette->grass_dark : s_palette->red);
    lv_obj_t *stats = text_at(s_scr, "", 31, 138,
                              &lv_font_montserrat_20, s_palette->ink);
    lv_label_set_text_fmt(stats, "FLOORS  %u\nPOP     %lu",
                          (unsigned)s_model.floor_count,
                          (unsigned long)s_model.population);
    text_at(s_scr, complete ? "OK  PLACE IN CITY" : "OK  TRY AGAIN", 29, 211,
            &lv_font_montserrat_14, s_palette->secondary);
    text_at(s_scr, "HOLD OK  MENU", 57, 270,
            &lv_font_montserrat_14, s_palette->on_art);
}

// Build the 4x4 map once; cursor and eligibility are refreshed on each input.
static void build_city(void)
{
    text_at(s_scr, "CITY MAP", 15, 11, &lv_font_montserrat_20, s_palette->title);
    lv_obj_t *map_backing = pixel_block(s_scr, 10, 58, 220, 211,
                                        s_palette->map_backing, s_palette->ink, 2);
    lv_obj_set_style_bg_opa(map_backing, LV_OPA_50, 0);
    for (uint8_t i = 0; i < TOWER_CITY_CELLS; i++) {
        int x = 23 + (i % 4) * 49;
        int y = 69 + (i / 4) * 48;
        s_city_cells[i] = pixel_block(s_scr, x, y, 43, 42, s_palette->grass,
                                       s_palette->ink, 2);
        lv_obj_t *label = text_at(s_city_cells[i], "", 5, 11,
                                  &lv_font_montserrat_14, s_palette->ink);
        if (s_model.city[i].type)
            lv_label_set_text_fmt(label, "%uF", (unsigned)s_model.city[i].type);
        else lv_label_set_text(label, "+");
    }
    s_city_hint = text_at(s_scr, "", 16, 266,
                          &lv_font_montserrat_14, s_palette->on_art);
    for (int i = 0; i < 4; i++) {
        s_tier_cards[i] = pixel_block(s_scr, 18 + i * 53, 288, 47, 24,
                                       s_palette->paper, s_palette->ink, 2);
        lv_obj_t *label = text_at(s_tier_cards[i], "", 8, 3,
                                  &lv_font_montserrat_14, s_palette->ink);
        lv_label_set_text_fmt(label, "%uF", (unsigned)CITY_TYPES[i]);
    }
}

// Eligible cells glow yellow; unlocked tier cards use the same repository theme.
static void refresh_city(void)
{
    if (!s_city_hint) return;
    bool placing = s_model.pending_type != 0;
    for (uint8_t i = 0; i < TOWER_CITY_CELLS; i++) {
        uint32_t color = s_palette->grass;
        if (s_model.city[i].type)
            color = s_palette->floors[(s_model.city[i].type / 10U - 1U) % 4U];
        else if (placing && !tower_model_can_place(&s_model, i, s_model.pending_type))
            color = s_palette->muted;
        else if (placing) color = s_palette->yellow;
        lv_obj_set_style_bg_color(s_city_cells[i], lv_color_hex(color), 0);
        lv_obj_set_style_border_color(s_city_cells[i],
            lv_color_hex(placing && i == s_city_cursor ? s_palette->red
                                                      : s_palette->ink), 0);
        lv_obj_set_style_border_width(s_city_cells[i],
                                       placing && i == s_city_cursor ? 4 : 2, 0);
    }
    for (uint8_t i = 0; i < 4; i++) {
        bool enabled = tower_model_can_build(&s_model, CITY_TYPES[i]);
        uint32_t color = !enabled ? s_palette->muted : i == s_tier_cursor && !placing
            ? s_palette->yellow : s_palette->paper;
        lv_obj_set_style_bg_color(s_tier_cards[i], lv_color_hex(color), 0);
        if (placing) lv_obj_add_flag(s_tier_cards[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_remove_flag(s_tier_cards[i], LV_OBJ_FLAG_HIDDEN);
    }
    if (placing) {
        lv_label_set_text_fmt(s_city_hint, "POP %lu  %uF  OK PLACE",
                              (unsigned long)s_model.city_population,
                              (unsigned)s_model.pending_type);
    } else {
        lv_label_set_text_fmt(s_city_hint, "CITY POP %lu  NEXT:",
                              (unsigned long)s_model.city_population);
    }
}

// Replace the active screen only on a phase transition or a deliberate selection.
// The previous screen owns every visual child and is deleted after the new load.
static void rebuild_screen(void)
{
    lv_obj_t *old = s_scr;
    // Keep only one full screen in LVGL's 24 KB pool. Building a replacement
    // before deleting the old screen exhausts the pool on menu navigation.
    if (old) lv_obj_delete(old);
    bool cover_phase = s_model.phase == TOWER_MENU || s_model.phase == TOWER_RESULT;
    const lv_image_dsc_t *art = cover_phase
        ? (s_theme == TOWER_THEME_LIGHT ? &TOWER_COVER_LIGHT : &TOWER_COVER)
        : (s_theme == TOWER_THEME_LIGHT ? &TOWER_BACKDROP_LIGHT : &TOWER_BACKDROP);
    s_scr = art_screen(art);
    s_battery_label = text_at(s_scr, "", 188, 13,
                              &lv_font_montserrat_14, s_palette->on_art);
    refresh_battery();
    s_playfield = s_active_floor = s_crane_pulley = s_crane_rope = NULL;
    s_foundation = s_stats_label = s_notice_label = s_city_hint = NULL;
    for (int i = 0; i < FLOOR_SLOTS; i++) s_floor_slots[i] = NULL;
    for (int i = 0; i < 3; i++) {
        s_menu_cards[i] = NULL;
        s_menu_labels[i] = NULL;
    }
    for (int i = 0; i < TOWER_CITY_CELLS; i++) s_city_cells[i] = NULL;
    for (int i = 0; i < 4; i++) s_tier_cards[i] = NULL;
    if (s_model.phase == TOWER_MENU) build_menu();
    else if (s_model.phase == TOWER_SWING || s_model.phase == TOWER_FALLING)
        build_play();
    else if (s_model.phase == TOWER_RESULT) build_result();
    else if (s_model.phase == TOWER_CITY) build_city();
    lv_screen_load(s_scr);
    s_drawn_phase = s_model.phase;
    s_rebuild = false;
    if (s_model.phase == TOWER_SWING || s_model.phase == TOWER_FALLING)
        refresh_play();
    if (s_model.phase == TOWER_CITY) refresh_city();
}

// Click releases a floor; holding OK always returns to this game's title menu.
// The button callback only enqueues and never mutates LVGL objects.
static void handle_input(tower_input_t input)
{
    if (input.button == BSP_BTN_OK && input.event == BSP_BTN_LONG) {
        if (s_model.phase != TOWER_MENU) {
            tower_model_show_menu(&s_model);
            s_rebuild = true;
            tower_audio_play(TOWER_SFX_NAV);
        }
        return;
    }
    if (input.event != BSP_BTN_CLICK) return;
    if (s_model.phase == TOWER_MENU) {
        if (input.button == BSP_BTN_UP || input.button == BSP_BTN_DOWN) {
            s_menu_choice = input.button == BSP_BTN_DOWN
                ? (uint8_t)((s_menu_choice + 1U) % 3U)
                : (uint8_t)((s_menu_choice + 2U) % 3U);
            refresh_menu_choice();
            tower_audio_play(TOWER_SFX_NAV);
        } else if (input.button == BSP_BTN_OK) {
            if (s_menu_choice == 2U) {
                s_theme = tower_theme_toggle(s_theme);
                s_palette = tower_theme_palette(s_theme);
                tower_theme_store_save(s_theme);
                s_rebuild = true;
                tower_audio_play(TOWER_SFX_NAV);
            } else {
                tower_mode_t mode = s_menu_choice == 0 ? TOWER_MODE_CITY : TOWER_MODE_QUICK;
                if (tower_model_start(&s_model, mode, mode == TOWER_MODE_CITY ? 10 : 0)) {
                    s_notice_text = NULL;
                    s_notice_until_ms = 0;
                    s_rebuild = true;
                    tower_audio_play(TOWER_SFX_START);
                }
            }
        }
    } else if (s_model.phase == TOWER_SWING) {
        if (input.button == BSP_BTN_OK && tower_model_release(&s_model)) {
            s_rebuild = true;
            tower_audio_play(TOWER_SFX_DROP);
        }
    } else if (s_model.phase == TOWER_RESULT && input.button == BSP_BTN_OK) {
        if (s_model.pending_type) {
            if (tower_model_open_city(&s_model)) {
                s_city_cursor = 0;
                for (uint8_t i = 0; i < TOWER_CITY_CELLS; i++) {
                    if (tower_model_can_place(&s_model, i, s_model.pending_type)) {
                        s_city_cursor = i;
                        break;
                    }
                }
                s_rebuild = true;
                tower_audio_play(TOWER_SFX_PLACE);
            }
        } else if (tower_model_start(&s_model, s_model.mode, s_model.target)) {
            s_notice_text = NULL;
            s_notice_until_ms = 0;
            s_rebuild = true;
            tower_audio_play(TOWER_SFX_START);
        }
    } else if (s_model.phase == TOWER_CITY) {
        if (s_model.pending_type) {
            if (input.button == BSP_BTN_UP) {
                s_city_cursor = (uint8_t)((s_city_cursor + 15U) % 16U);
                tower_audio_play(TOWER_SFX_NAV);
            } else if (input.button == BSP_BTN_DOWN) {
                s_city_cursor = (uint8_t)((s_city_cursor + 1U) % 16U);
                tower_audio_play(TOWER_SFX_NAV);
            } else if (input.button == BSP_BTN_OK && tower_model_place(&s_model, s_city_cursor)) {
                s_tier_cursor = 0;
                tower_audio_play(TOWER_SFX_PLACE);
            }
            refresh_city();
        } else {
            if (input.button == BSP_BTN_UP) {
                s_tier_cursor = (uint8_t)((s_tier_cursor + 3U) % 4U);
                tower_audio_play(TOWER_SFX_NAV);
            } else if (input.button == BSP_BTN_DOWN) {
                s_tier_cursor = (uint8_t)((s_tier_cursor + 1U) % 4U);
                tower_audio_play(TOWER_SFX_NAV);
            }
            else if (input.button == BSP_BTN_OK &&
                     tower_model_start(&s_model, TOWER_MODE_CITY, CITY_TYPES[s_tier_cursor])) {
                s_notice_text = NULL;
                s_notice_until_ms = 0;
                s_rebuild = true;
                tower_audio_play(TOWER_SFX_START);
            }
            if (!s_rebuild) refresh_city();
        }
    }
}

// LVGL owns this timer callback. It drains queued keys and drives one fixed-step-ish
// model update per frame; elapsed time is capped in the pure model after stalls.
static void tick(lv_timer_t *timer)
{
    (void)timer;
    if (s_battery_soc == -2) {
        s_battery_soc = bsp_battery_soc();
        refresh_battery();
    }
    if (s_intro_active) {
        uint64_t time_ms = now_ms();
        uint32_t elapsed = (uint32_t)(time_ms - s_intro_start_ms);
        if (elapsed > INTRO_MS) elapsed = INTRO_MS;
        lv_obj_set_width(s_loading_fill, 1 + (int)(190U * elapsed / INTRO_MS));
        tower_input_t skipped;
        while (s_input_queue && xQueueReceive(s_input_queue, &skipped, 0) == pdTRUE) {
            if (skipped.button == BSP_BTN_OK && skipped.event == BSP_BTN_CLICK &&
                elapsed >= 600U) elapsed = INTRO_MS;
        }
        if (elapsed < INTRO_MS) return;
        s_intro_active = false;
        s_loading_fill = NULL;
        s_last_tick_ms = time_ms;
        rebuild_screen();
        return;
    }
    tower_input_t input;
    while (s_input_queue && xQueueReceive(s_input_queue, &input, 0) == pdTRUE)
        handle_input(input);
    uint64_t time_ms = now_ms();
    uint32_t elapsed = (uint32_t)(time_ms - s_last_tick_ms);
    s_last_tick_ms = time_ms;
    tower_event_t event = tower_model_tick(&s_model, elapsed);
    switch (event) {
    case TOWER_LANDED:
        tower_audio_play(s_model.last_grade == TOWER_GRADE_PERFECT ?
                         TOWER_SFX_PERFECT : TOWER_SFX_LAND);
        break;
    case TOWER_MISSED: tower_audio_play(TOWER_SFX_MISS); break;
    case TOWER_COMPLETE: tower_audio_play(TOWER_SFX_WIN); break;
    case TOWER_GAME_OVER: tower_audio_play(TOWER_SFX_LOSE); break;
    default: break;
    }
    if (event == TOWER_LANDED || event == TOWER_MISSED) {
        if (event == TOWER_MISSED) s_notice_text = "MISSED!";
        else if (s_model.last_grade == TOWER_GRADE_PERFECT)
            s_notice_text = "PERFECT!";
        else s_notice_text = "NICE!";
        s_notice_until_ms = time_ms + 900U;
        if (s_notice_label) lv_label_set_text(s_notice_label, s_notice_text);
    }
    if (s_rebuild || s_model.phase != s_drawn_phase) rebuild_screen();
    else if (s_model.phase == TOWER_SWING || s_model.phase == TOWER_FALLING)
        refresh_play();
}

void demo_tower_enter(tower_theme_t theme)
{
    s_theme = theme;
    s_palette = tower_theme_palette(theme);
    tower_model_init(&s_model);
    s_menu_choice = s_city_cursor = s_tier_cursor = 0;
    s_battery_soc = -2;
    s_notice_until_ms = 0;
    s_notice_text = NULL;
    s_input_queue = xQueueCreate(8, sizeof(tower_input_t));
    s_last_tick_ms = now_ms();
    s_intro_start_ms = s_last_tick_ms;
    s_intro_active = true;
    s_scr = NULL;
    s_battery_label = NULL;
    s_rebuild = false;
    build_splash();
    s_timer = lv_timer_create(tick, TICK_MS, NULL);
    (void)tower_audio_start();
}

void demo_tower_exit(void)
{
    // main.c holds the LVGL lock. Stop the only UI callback before deleting objects.
    if (s_timer) { lv_timer_delete(s_timer); s_timer = NULL; }
    tower_audio_stop();
    if (s_input_queue) { vQueueDelete(s_input_queue); s_input_queue = NULL; }
    if (s_scr) { lv_obj_delete(s_scr); s_scr = NULL; }
    s_battery_label = s_playfield = s_active_floor = NULL;
    s_crane_pulley = s_crane_rope = s_foundation = NULL;
    s_stats_label = s_notice_label = s_city_hint = NULL;
    s_loading_fill = NULL;
    s_intro_active = false;
}

void demo_tower_key(bsp_btn_t button, bsp_btn_ev_t event)
{
    // Queue send is nonblocking; the LVGL timer performs all visual work.
    if (!s_input_queue ||
        (event != BSP_BTN_CLICK &&
         !(button == BSP_BTN_OK && event == BSP_BTN_LONG))) return;
    tower_input_t input = { .button = button, .event = event };
    (void)xQueueSend(s_input_queue, &input, 0);
}
