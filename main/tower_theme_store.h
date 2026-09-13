// Persist the player's visual preference in the ordinary NVS partition.
#pragma once

#include "tower_theme.h"

// Called once during boot, before the LVGL screen and audio worker start.
// Missing or unreadable NVS safely defaults to the original dark theme.
tower_theme_t tower_theme_store_init(void);

// Nonblocking for the LVGL task; the worker coalesces quick toggles before commit.
void tower_theme_store_save(tower_theme_t theme);
