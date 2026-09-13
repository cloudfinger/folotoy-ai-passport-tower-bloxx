#pragma once

#include "lvgl.h"

// Warm pixel-city palette shared by the menu and every firmware page.
#define UI_SKY        0xF2DFC5  // parchment backdrop
#define UI_SKY_DARK   0x78484A  // secondary copy
#define UI_INK        0x30262C
#define UI_PAPER      0xFFF1DB
#define UI_GRASS      0xD98560  // terracotta surfaces
#define UI_GRASS_DARK 0x88434A
#define UI_YELLOW     0xF0BD5B
#define UI_ORANGE     0xD97D4D
#define UI_RED        0xB34448
#define UI_MUTED      0xD5C2AD
#define UI_FOOTER     0x423039
#define UI_LIGHT      0xFFF1DB

lv_obj_t *ui_pixel_screen_create(const char *title);
lv_obj_t *ui_pixel_panel_create(lv_obj_t *parent, int x, int y, int w, int h,
                                uint32_t color);
lv_obj_t *ui_pixel_label(lv_obj_t *parent, const char *text,
                         const lv_font_t *font, uint32_t color);
lv_obj_t *ui_pixel_mascot_create(lv_obj_t *parent, int x, int y);
void ui_pixel_mascot_jump(lv_obj_t *mascot);
void ui_pixel_set_selected(lv_obj_t *panel, bool selected, bool enabled);
