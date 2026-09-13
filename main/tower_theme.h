// Pure visual theme data for the two Tower Bloxx colorways.
#pragma once

#include <stdint.h>

typedef enum {
    TOWER_THEME_DARK = 0,
    TOWER_THEME_LIGHT = 1,
} tower_theme_t;

typedef struct {
    uint32_t ink;
    uint32_t paper;
    uint32_t grass;
    uint32_t grass_dark;
    uint32_t yellow;
    uint32_t orange;
    uint32_t red;
    uint32_t muted;
    uint32_t footer;
    uint32_t on_art;
    uint32_t on_footer;
    uint32_t secondary;
    uint32_t title;
    uint32_t menu_bg;
    uint32_t menu_selected;
    uint32_t menu_border;
    uint32_t menu_selected_border;
    uint32_t menu_text;
    uint32_t menu_selected_text;
    uint32_t map_backing;
    uint32_t window_bright;
    uint32_t window_dark;
    uint32_t floors[4];
} tower_palette_t;

tower_theme_t tower_theme_from_saved(uint8_t value);
tower_theme_t tower_theme_toggle(tower_theme_t theme);
const tower_palette_t *tower_theme_palette(tower_theme_t theme);
