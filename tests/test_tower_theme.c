#include "tower_theme.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static int luma(uint32_t color)
{
    return (int)(((color >> 16 & 255U) * 299U +
                  (color >> 8 & 255U) * 587U +
                  (color & 255U) * 114U) / 1000U);
}

static int luma_gap(uint32_t a, uint32_t b)
{
    int gap = luma(a) - luma(b);
    return gap < 0 ? -gap : gap;
}

static void test_theme_choices_and_readability(void)
{
    assert(tower_theme_from_saved(255) == TOWER_THEME_DARK);
    assert(tower_theme_from_saved(0) == TOWER_THEME_DARK);
    assert(tower_theme_from_saved(1) == TOWER_THEME_LIGHT);
    assert(tower_theme_toggle(TOWER_THEME_DARK) == TOWER_THEME_LIGHT);
    assert(tower_theme_toggle(TOWER_THEME_LIGHT) == TOWER_THEME_DARK);
    for (int theme = TOWER_THEME_DARK; theme <= TOWER_THEME_LIGHT; theme++) {
        const tower_palette_t *p = tower_theme_palette((tower_theme_t)theme);
        assert(luma_gap(p->menu_bg, p->menu_text) >= 100);
        assert(luma_gap(p->menu_selected, p->menu_selected_text) >= 100);
        assert(luma_gap(p->footer, p->on_footer) >= 100);
        assert(luma_gap(p->paper, p->ink) >= 100);
        assert(luma_gap(p->grass, p->ink) >= 90);
    }
}

int main(void)
{
    test_theme_choices_and_readability();
    puts("tower theme: PASS");
    return 0;
}
