#include "tower_audio.h"

#include "bsp_audio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "tower_audio";

typedef struct {
    bool stop;
    tower_sfx_t cue;
} audio_command_t;

static QueueHandle_t s_commands;
static SemaphoreHandle_t s_stopped;
static TaskHandle_t s_worker;

static void audio_worker(void *arg)
{
    (void)arg;
    bool ready = bsp_audio_init() == ESP_OK &&
                 bsp_audio_set_format(TOWER_SOUND_SAMPLE_RATE, 16, 1) == ESP_OK;
    if (ready) {
        bsp_audio_set_volume(60);
        ESP_LOGI(TAG, "BGM ready: %u Hz mono, free internal=%u, largest=%u",
                 TOWER_SOUND_SAMPLE_RATE,
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    } else {
        ESP_LOGE(TAG, "Audio unavailable; game continues silently");
    }

    tower_sound_t soundtrack;
    tower_sound_init(&soundtrack);
    int16_t pcm[TOWER_SOUND_CHUNK_SAMPLES];
    uint64_t last_feed_us = 0;
    uint32_t largest_feed_gap_us = 0;
    uint32_t late_feeds = 0;
    uint64_t report_after_us = (uint64_t)esp_timer_get_time() + 10000000U;
    for (;;) {
        audio_command_t command;
        if (!ready) {
            // A failed codec never blocks the game. Retain the worker only so
            // demo_tower_exit() can stop it through the same handshake.
            xQueueReceive(s_commands, &command, portMAX_DELAY);
            if (command.stop) break;
            continue;
        }
        while (xQueueReceive(s_commands, &command, 0) == pdTRUE) {
            if (command.stop) goto stopped;
            tower_sound_cue(&soundtrack, command.cue);
        }
        tower_sound_render(&soundtrack, pcm, TOWER_SOUND_CHUNK_SAMPLES);
        uint64_t feed_us = (uint64_t)esp_timer_get_time();
        if (last_feed_us) {
            uint32_t gap_us = (uint32_t)(feed_us - last_feed_us);
            if (gap_us > largest_feed_gap_us) largest_feed_gap_us = gap_us;
            if (gap_us > 90000U) late_feeds++;
        }
        last_feed_us = feed_us;
        if (bsp_audio_write(pcm, sizeof(pcm)) != ESP_OK) {
            ESP_LOGE(TAG, "PCM delivery failed; muting soundtrack");
            ready = false;
        }
        if (feed_us >= report_after_us) {
            if (late_feeds)
                ESP_LOGW(TAG, "PCM feed: %u late chunks, max gap %u us",
                         (unsigned)late_feeds, (unsigned)largest_feed_gap_us);
            late_feeds = largest_feed_gap_us = 0;
            report_after_us = feed_us + 10000000U;
        }
    }
stopped:
    xSemaphoreGive(s_stopped);
    vTaskDelete(NULL);
}

bool tower_audio_start(void)
{
    if (s_worker) return true;
    s_commands = xQueueCreate(8, sizeof(audio_command_t));
    s_stopped = xSemaphoreCreateBinary();
    if (!s_commands || !s_stopped ||
        xTaskCreate(audio_worker, "tower_audio", 4096, NULL, 5, &s_worker) != pdPASS) {
        ESP_LOGE(TAG, "Could not start soundtrack worker");
        if (s_commands) vQueueDelete(s_commands);
        if (s_stopped) vSemaphoreDelete(s_stopped);
        s_commands = NULL;
        s_stopped = NULL;
        s_worker = NULL;
        return false;
    }
    return true;
}

void tower_audio_play(tower_sfx_t cue)
{
    if (!s_commands || cue == TOWER_SFX_NONE) return;
    audio_command_t command = { .stop = false, .cue = cue };
    (void)xQueueSend(s_commands, &command, 0);
}

void tower_audio_stop(void)
{
    if (!s_worker) return;
    audio_command_t command = { .stop = true, .cue = TOWER_SFX_NONE };
    if (xQueueSendToFront(s_commands, &command, pdMS_TO_TICKS(100)) != pdTRUE ||
        xSemaphoreTake(s_stopped, pdMS_TO_TICKS(600)) != pdTRUE) {
        ESP_LOGW(TAG, "Soundtrack worker did not stop in time");
        return;
    }
    vQueueDelete(s_commands);
    vSemaphoreDelete(s_stopped);
    s_commands = NULL;
    s_stopped = NULL;
    s_worker = NULL;
}
