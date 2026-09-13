// Original procedural Tower Bloxx soundtrack; no ESP-IDF or LVGL dependency.
#pragma once

#include <stddef.h>
#include <stdint.h>

#define TOWER_SOUND_SAMPLE_RATE 16000U
#define TOWER_SOUND_CHUNK_SAMPLES 480U

typedef enum {
    TOWER_SFX_NONE,
    TOWER_SFX_NAV,
    TOWER_SFX_START,
    TOWER_SFX_DROP,
    TOWER_SFX_LAND,
    TOWER_SFX_PERFECT,
    TOWER_SFX_MISS,
    TOWER_SFX_WIN,
    TOWER_SFX_LOSE,
    TOWER_SFX_PLACE,
} tower_sfx_t;

typedef struct {
    uint32_t lead_phase;
    uint32_t bass_phase;
    uint32_t effect_phase;
    uint32_t lead_step;
    uint32_t bass_step;
    uint32_t effect_step;
    uint16_t music_step_age;
    uint16_t bass_age;
    uint16_t effect_note_age;
    uint16_t effect_note_length;
    uint16_t effect_level;
    uint16_t music_gain_q8;
    uint8_t music_step;
    uint8_t effect_note;
    tower_sfx_t active_cue;
} tower_sound_t;

void tower_sound_init(tower_sound_t *sound);
void tower_sound_cue(tower_sound_t *sound, tower_sfx_t cue);
void tower_sound_render(tower_sound_t *sound, int16_t *output, size_t samples);
