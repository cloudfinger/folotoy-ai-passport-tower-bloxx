#include "tower_theme.h"

// Dark preserves the original dusk palette and brick-floor colors.
static const tower_palette_t DARK = {
    .ink = 0x30262C,
    .paper = 0xFFF1DB,
    .grass = 0xD98560,
    .grass_dark = 0x88434A,
    .yellow = 0xF0BD5B,
    .orange = 0xD97D4D,
    .red = 0xB34448,
    .muted = 0xD5C2AD,
    .footer = 0x423039,
    .on_art = 0xFFF1DB,
    .on_footer = 0xFFF1DB,
    .secondary = 0x78484A,
    .title = 0xF0BD5B,
    .menu_bg = 0x423039,
    .menu_selected = 0xF0BD5B,
    .menu_border = 0xD5C2AD,
    .menu_selected_border = 0xFFF1DB,
    .menu_text = 0xFFF1DB,
    .menu_selected_text = 0x30262C,
    .map_backing = 0x423039,
    .window_bright = 0xF9D698,
    .window_dark = 0x88434A,
    .floors = { 0xC66B50, 0xD8995B, 0xA75B60, 0xB77A57 },
};

// Daylight uses ink-on-cream surfaces over the matching blue-sky artwork.
static const tower_palette_t LIGHT = {
    .ink = 0x243443,
    .paper = 0xFFF9EC,
    .grass = 0xCBE4D6,
    .grass_dark = 0x397C70,
    .yellow = 0xF4C66B,
    .orange = 0xD98252,
    .red = 0xB74E58,
    .muted = 0xBED0D4,
    .footer = 0xE9F3F0,
    .on_art = 0x243443,
    .on_footer = 0x243443,
    .secondary = 0x456574,
    .title = 0x843D3F,
    .menu_bg = 0xFFF9EC,
    .menu_selected = 0x317F9F,
    .menu_border = 0x6A8790,
    .menu_selected_border = 0x243443,
    .menu_text = 0x243443,
    .menu_selected_text = 0xFFFFFF,
    .map_backing = 0xEAF4F2,
    .window_bright = 0xF5FDFF,
    .window_dark = 0x6F91A2,
    .floors = { 0xE5A074, 0xE9BB90, 0xC98E90, 0xD4AF82 },
};

tower_theme_t tower_theme_from_saved(uint8_t value)
{
    return value == TOWER_THEME_LIGHT ? TOWER_THEME_LIGHT : TOWER_THEME_DARK;
}

tower_theme_t tower_theme_toggle(tower_theme_t theme)
{
    return theme == TOWER_THEME_LIGHT ? TOWER_THEME_DARK : TOWER_THEME_LIGHT;
}

const tower_palette_t *tower_theme_palette(tower_theme_t theme)
{
    return theme == TOWER_THEME_LIGHT ? &LIGHT : &DARK;
}
