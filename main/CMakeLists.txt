idf_component_register(
    SRCS "main.c"
         "ui_pixel.c"
         "ui_pixel_math.c"
         "demo_tower.c"
         "tower_model.c"
         "tower_sound.c"
         "tower_audio.c"
         "tower_theme.c"
         "tower_theme_store.c"
    INCLUDE_DIRS "."
    EMBED_FILES "tower_cover.rgb565" "tower_backdrop.rgb565"
                "tower_cover_light.rgb565" "tower_backdrop_light.rgb565"
    REQUIRES bsp esp_timer nvs_flash
)
