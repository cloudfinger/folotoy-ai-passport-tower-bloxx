#include "ui_pixel.h"

static void start_blink(lv_obj_t *eye);

static lv_obj_t *block(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    return obj;
}

lv_obj_t *ui_pixel_label(lv_obj_t *parent, const char *text,
                         const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

lv_obj_t *ui_pixel_screen_create(const char *title)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_SKY), 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);

    // A low silhouette and construction stripe replace the original sky/grass.
    block(scr, 0, 279, 34, 7, UI_MUTED);
    block(scr, 34, 271, 34, 15, UI_MUTED);
    block(scr, 68, 276, 27, 10, UI_MUTED);
    block(scr, 95, 269, 31, 17, UI_MUTED);
    block(scr, 126, 278, 37, 8, UI_MUTED);
    block(scr, 163, 273, 34, 13, UI_MUTED);
    block(scr, 197, 279, 43, 7, UI_MUTED);
    block(scr, 0, 286, 240, 34, UI_FOOTER);
    block(scr, 0, 286, 240, 4, UI_ORANGE);
    for (int x = 4; x < 240; x += 30)
        block(scr, x, 286, 13, 4, UI_YELLOW);

    block(scr, 10, 12, 151, 33, UI_INK);
    lv_obj_t *plate = block(scr, 5, 8, 151, 33, UI_YELLOW);
    lv_obj_set_style_border_color(plate, lv_color_hex(UI_INK), 0);
    lv_obj_set_style_border_width(plate, 2, 0);
    lv_obj_t *heading = ui_pixel_label(plate, title, &lv_font_montserrat_20, UI_INK);
    lv_obj_center(heading);
    // Small sunset mark balances the heading without occupying battery space.
    block(scr, 195, 11, 27, 27, UI_INK);
    block(scr, 198, 14, 21, 21, UI_ORANGE);
    block(scr, 203, 19, 11, 11, UI_YELLOW);
    return scr;
}

lv_obj_t *ui_pixel_panel_create(lv_obj_t *parent, int x, int y, int w, int h,
                                uint32_t color)
{
    block(parent, x + 4, y + 5, w, h, UI_GRASS_DARK);
    lv_obj_t *panel = block(parent, x, y, w, h, color);
    lv_obj_set_style_border_color(panel, lv_color_hex(UI_INK), 0);
    lv_obj_set_style_border_width(panel, 3, 0);
    lv_obj_set_style_pad_all(panel, 7, 0);
    return panel;
}

lv_obj_t *ui_pixel_mascot_create(lv_obj_t *parent, int x, int y)
{
    lv_obj_t *m = lv_obj_create(parent);
    lv_obj_remove_flag(m, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(m, x, y);
    lv_obj_set_size(m, 38, 48);
    lv_obj_set_style_bg_opa(m, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(m, 0, 0);
    lv_obj_set_style_pad_all(m, 0, 0);

    // Tiny site foreman: hardhat, face, jacket, and two animated eyes.
    block(m, 7, 5, 24, 7, UI_YELLOW);
    block(m, 3, 11, 32, 5, UI_INK);
    block(m, 5, 12, 28, 3, UI_YELLOW);
    block(m, 5, 16, 28, 16, UI_INK);
    block(m, 8, 18, 22, 11, UI_PAPER);
    lv_obj_t *left_eye = block(m, 12, 21, 3, 5, UI_INK);
    lv_obj_t *right_eye = block(m, 23, 21, 3, 5, UI_INK);
    block(m, 10, 32, 18, 3, UI_YELLOW);
    block(m, 6, 35, 26, 10, UI_RED);
    block(m, 18, 35, 3, 10, UI_YELLOW);
    block(m, 5, 45, 11, 3, UI_INK);
    block(m, 22, 45, 11, 3, UI_INK);
    start_blink(left_eye);
    start_blink(right_eye);
    return m;
}

static void jump_y(void *obj, int32_t value)
{
    lv_obj_set_y((lv_obj_t *)obj, value);
}

static void blink_eye(void *obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)value, 0);
}

static void start_blink(lv_obj_t *eye)
{
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, eye);
    lv_anim_set_exec_cb(&anim, blink_eye);
    lv_anim_set_values(&anim, LV_OPA_COVER, LV_OPA_20);
    lv_anim_set_duration(&anim, 70);
    lv_anim_set_playback_duration(&anim, 70);
    lv_anim_set_repeat_delay(&anim, 1700);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&anim, lv_anim_path_step);
    lv_anim_start(&anim);
}

void ui_pixel_mascot_jump(lv_obj_t *mascot)
{
    if (!mascot) return;
    int y = lv_obj_get_y(mascot);
    lv_anim_delete(mascot, jump_y);
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, mascot);
    lv_anim_set_exec_cb(&anim, jump_y);
    lv_anim_set_values(&anim, y, y - 5);
    lv_anim_set_duration(&anim, 110);
    lv_anim_set_playback_duration(&anim, 140);
    lv_anim_set_path_cb(&anim, lv_anim_path_step);
    lv_anim_start(&anim);
}

void ui_pixel_set_selected(lv_obj_t *panel, bool selected, bool enabled)
{
    uint32_t color = !enabled ? UI_MUTED : (selected ? UI_YELLOW : UI_PAPER);
    lv_obj_set_style_bg_color(panel, lv_color_hex(color), 0);
    lv_obj_set_style_border_color(panel,
        lv_color_hex(selected ? UI_RED : UI_INK), 0);
}
