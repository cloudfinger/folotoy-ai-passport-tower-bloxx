// Standalone Tower Bloxx page. Input is queued from the button callback.
#pragma once

#include "bsp_button.h"
#include "tower_theme.h"

void demo_tower_enter(tower_theme_t theme);
void demo_tower_exit(void);
void demo_tower_key(bsp_btn_t button, bsp_btn_ev_t event);
