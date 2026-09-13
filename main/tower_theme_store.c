#include "tower_theme_store.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "tower_theme";
static QueueHandle_t s_pending;
static nvs_handle_t s_nvs;

static void save_worker(void *arg)
{
    (void)arg;
    uint8_t value;
    for (;;) {
        xQueueReceive(s_pending, &value, portMAX_DELAY);
        // Only the last of a quick series of toggles needs a Flash write.
        uint8_t newer;
        while (xQueueReceive(s_pending, &newer, pdMS_TO_TICKS(250)) == pdTRUE)
            value = newer;
        esp_err_t err = nvs_set_u8(s_nvs, "style", value);
        if (err == ESP_OK) err = nvs_commit(s_nvs);
        if (err != ESP_OK)
            ESP_LOGW(TAG, "Could not save visual theme: %s", esp_err_to_name(err));
    }
}

tower_theme_t tower_theme_store_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        // Never erase the user's NVS partition as a recovery shortcut.
        ESP_LOGW(TAG, "NVS unavailable; using dark theme: %s", esp_err_to_name(err));
        return TOWER_THEME_DARK;
    }
    err = nvs_open("tower_game", NVS_READWRITE, &s_nvs);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Theme storage unavailable: %s", esp_err_to_name(err));
        return TOWER_THEME_DARK;
    }
    uint8_t saved = TOWER_THEME_DARK;
    err = nvs_get_u8(s_nvs, "style", &saved);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        ESP_LOGW(TAG, "Could not read saved theme: %s", esp_err_to_name(err));
    tower_theme_t theme = tower_theme_from_saved(saved);
    s_pending = xQueueCreate(1, sizeof(uint8_t));
    if (!s_pending || xTaskCreate(save_worker, "tower_theme", 3072, NULL, 2, NULL) != pdPASS) {
        ESP_LOGW(TAG, "Theme can be changed, but persistence is unavailable");
        if (s_pending) vQueueDelete(s_pending);
        s_pending = NULL;
        nvs_close(s_nvs);
    }
    ESP_LOGI(TAG, "Visual theme: %s", theme == TOWER_THEME_LIGHT ? "light" : "dark");
    return theme;
}

void tower_theme_store_save(tower_theme_t theme)
{
    if (!s_pending) return;
    uint8_t value = (uint8_t)theme;
    (void)xQueueOverwrite(s_pending, &value);
}
