// Standalone Tower Bloxx entry point for the AI Passport.
// The button task only queues inputs; the LVGL task owns every game screen.
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "demo.h"
#include "tower_theme_store.h"

#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "tower_boot";

static void on_key(bsp_btn_t button, bsp_btn_ev_t event, void *user)
{
    (void)user;
    demo_tower_key(button, event);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Tower Bloxx");
    tower_theme_t theme = tower_theme_store_init();
    bsp_i2c_init();
    if (bsp_display_init() != ESP_OK) {
        ESP_LOGE(TAG, "Display failed (MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(0);
    if (!bsp_lvgl_init()) {
        ESP_LOGE(TAG, "LVGL initialization failed");
        return;
    }
    if (bsp_button_init(on_key, NULL) != ESP_OK)
        ESP_LOGE(TAG, "Button initialization failed");
    if (bsp_battery_init() != ESP_OK)
        ESP_LOGW(TAG, "Battery status unavailable");

    if (!bsp_lvgl_lock(1000)) {
        ESP_LOGE(TAG, "Could not acquire LVGL lock for opening screen");
        return;
    }
    demo_tower_enter(theme);
    lv_refr_now(NULL);
    bsp_lvgl_unlock();
    bsp_display_backlight(100);
    ESP_LOGI(TAG, "Tower Bloxx cover ready");
}
