#include "tower_sound.h"

#include <string.h>

#define MUSIC_STEP_SAMPLES 3200U // Eighth notes at 150 BPM.
#define BASS_PULSE_SAMPLES (MUSIC_STEP_SAMPLES * 4U)

typedef struct {
    uint16_t frequency_hz;
    uint16_t duration_ms;
} effect_note_t;

typedef struct {
    const effect_note_t *notes;
    uint8_t count;
    uint8_t priority;
    uint16_t level;
} effect_plan_t;

// A minor pentatonic, with F/C/G bass changes: a compact original 6.4 s loop.
static const uint16_t MELODY[32] = {
    440, 0, 523, 659, 784, 659, 587, 523,
    440, 523, 587, 659, 784, 659, 523, 0,
    659, 784, 880, 784, 659, 587, 523, 587,
    440, 523, 659, 587, 523, 440, 0, 0,
};
static const uint16_t BASS_ROOTS[4] = { 220, 175, 262, 196 };

static const effect_note_t FX_NAV[] = { { 698, 55 } };
static const effect_note_t FX_START[] = {
    { 523, 70 }, { 659, 70 }, { 784, 120 },
};
static const effect_note_t FX_DROP[] = {
    { 659, 60 }, { 440, 70 }, { 262, 90 },
};
static const effect_note_t FX_LAND[] = { { 262, 90 }, { 196, 140 } };
static const effect_note_t FX_PERFECT[] = {
    { 659, 90 }, { 880, 90 }, { 1175, 190 },
};
static const effect_note_t FX_MISS[] = {
    { 330, 120 }, { 220, 140 }, { 147, 160 },
};
static const effect_note_t FX_WIN[] = {
    { 523, 90 }, { 659, 90 }, { 784, 90 }, { 1046, 240 },
};
static const effect_note_t FX_LOSE[] = {
    { 392, 110 }, { 311, 110 }, { 220, 220 },
};
static const effect_note_t FX_PLACE[] = { { 784, 60 }, { 1046, 160 } };

static effect_plan_t effect_plan(tower_sfx_t cue)
{
    switch (cue) {
    case TOWER_SFX_NAV:     return (effect_plan_t){ FX_NAV, 1, 1, 1300 };
    case TOWER_SFX_START:   return (effect_plan_t){ FX_START, 3, 3, 3400 };
    case TOWER_SFX_DROP:    return (effect_plan_t){ FX_DROP, 3, 2, 4300 };
    case TOWER_SFX_LAND:    return (effect_plan_t){ FX_LAND, 2, 2, 4500 };
    case TOWER_SFX_PERFECT: return (effect_plan_t){ FX_PERFECT, 3, 4, 5000 };
    case TOWER_SFX_MISS:    return (effect_plan_t){ FX_MISS, 3, 4, 4800 };
    case TOWER_SFX_WIN:     return (effect_plan_t){ FX_WIN, 4, 5, 5000 };
    case TOWER_SFX_LOSE:    return (effect_plan_t){ FX_LOSE, 3, 5, 5000 };
    case TOWER_SFX_PLACE:  return (effect_plan_t){ FX_PLACE, 2, 3, 3400 };
    default:               return (effect_plan_t){ NULL, 0, 0, 0 };
    }
}

// DDS keeps all per-sample work integer-only; this division runs on note changes.
static uint32_t phase_step(uint16_t frequency_hz)
{
    return (uint32_t)(((uint64_t)frequency_hz << 32) / TOWER_SOUND_SAMPLE_RATE);
}

static int32_t triangle(uint32_t phase)
{
    uint32_t position = phase >> 16;
    return (int32_t)((position < 32768U ? position : 65535U - position) * 2U)
           - 32767;
}

static void effect_begin_note(tower_sound_t *sound)
{
    effect_plan_t plan = effect_plan(sound->active_cue);
    if (sound->effect_note >= plan.count) {
        sound->active_cue = TOWER_SFX_NONE;
        sound->effect_step = 0;
        return;
    }
    effect_note_t note = plan.notes[sound->effect_note];
    sound->effect_step = phase_step(note.frequency_hz);
    sound->effect_note_length = (uint16_t)(note.duration_ms * 16U);
    sound->effect_note_age = 0;
    sound->effect_phase = 0;
}

void tower_sound_init(tower_sound_t *sound)
{
    if (!sound) return;
    memset(sound, 0, sizeof(*sound));
    sound->lead_step = phase_step(MELODY[0]);
    sound->bass_step = phase_step(BASS_ROOTS[0]);
    sound->music_gain_q8 = 256;
}

void tower_sound_cue(tower_sound_t *sound, tower_sfx_t cue)
{
    if (!sound) return;
    effect_plan_t next = effect_plan(cue);
    if (!next.count) return;
    if (sound->active_cue != TOWER_SFX_NONE &&
        next.priority < effect_plan(sound->active_cue).priority) return;
    sound->active_cue = cue;
    sound->effect_level = next.level;
    sound->effect_note = 0;
    effect_begin_note(sound);
}

void tower_sound_render(tower_sound_t *sound, int16_t *output, size_t samples)
{
    if (!sound || !output) return;
    for (size_t i = 0; i < samples; i++) {
        // The lead has a short attack/release, so each note remains distinct.
        int32_t lead = 0;
        if (sound->lead_step) {
            uint32_t envelope = 256;
            if (sound->music_step_age < 96U)
                envelope = sound->music_step_age * 256U / 96U;
            else if (sound->music_step_age > 2500U)
                envelope = (MUSIC_STEP_SAMPLES - sound->music_step_age) * 256U / 700U;
            sound->lead_phase += sound->lead_step;
            int32_t pulse = (sound->lead_phase >> 30) == 0U ? 3 : -1;
            lead = pulse * 900 * (int32_t)envelope / 256;
        }

        sound->bass_phase += sound->bass_step;
        int32_t bass = triangle(sound->bass_phase) * 1200 / 32767;
        bass = bass * (int32_t)(BASS_PULSE_SAMPLES - sound->bass_age) /
               (int32_t)BASS_PULSE_SAMPLES;
        if (sound->bass_age < BASS_PULSE_SAMPLES) sound->bass_age++;

        int32_t effect = 0;
        if (sound->active_cue != TOWER_SFX_NONE) {
            if (sound->effect_note_age >= sound->effect_note_length) {
                sound->effect_note++;
                effect_begin_note(sound);
            }
            if (sound->active_cue != TOWER_SFX_NONE) {
                uint32_t envelope = 256;
                if (sound->effect_note_age < 40U)
                    envelope = sound->effect_note_age * 256U / 40U;
                else if (sound->effect_note_age + 120U > sound->effect_note_length)
                    envelope = (sound->effect_note_length - sound->effect_note_age) *
                               256U / 120U;
                sound->effect_phase += sound->effect_step;
                int32_t wave = (sound->effect_phase >> 30) == 0U ? 3 : -1;
                effect = wave * sound->effect_level * (int32_t)envelope / 256;
                sound->effect_note_age++;
            }
        }

        // Duck the BGM for meaningful actions; smooth both edges to prevent
        // gain-change clicks while leaving the light menu beep less intrusive.
        uint16_t target_gain = sound->active_cue == TOWER_SFX_NONE ? 256U :
                               sound->active_cue == TOWER_SFX_NAV ? 192U : 64U;
        if (sound->music_gain_q8 < target_gain) {
            uint16_t next_gain = sound->music_gain_q8 + 4U;
            sound->music_gain_q8 = next_gain > target_gain ? target_gain : next_gain;
        } else if (sound->music_gain_q8 > target_gain) {
            uint16_t next_gain = sound->music_gain_q8 - 4U;
            sound->music_gain_q8 = next_gain < target_gain ? target_gain : next_gain;
        }
        int32_t mixed = (lead + bass) * sound->music_gain_q8 / 256 + effect;
        if (mixed > INT16_MAX) mixed = INT16_MAX;
        if (mixed < INT16_MIN) mixed = INT16_MIN;
        output[i] = (int16_t)mixed;

        if (++sound->music_step_age >= MUSIC_STEP_SAMPLES) {
            sound->music_step_age = 0;
            sound->music_step = (uint8_t)((sound->music_step + 1U) % 32U);
            sound->lead_step = phase_step(MELODY[sound->music_step]);
            if (sound->music_step % 4U == 0U) {
                sound->bass_age = 0;
                sound->bass_step = phase_step(BASS_ROOTS[sound->music_step / 8U]);
            }
        }
    }
}
