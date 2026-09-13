#include "tower_sound.h"

#include <assert.h>
#include <stdio.h>

// Compare the same musical moment, so each gameplay cue has to stand above
// the soundtrack rather than merely produce different PCM samples.
static uint64_t early_energy(tower_sfx_t cue)
{
    tower_sound_t sound;
    int16_t buffer[TOWER_SOUND_CHUNK_SAMPLES];
    uint64_t energy = 0;
    tower_sound_init(&sound);
    tower_sound_cue(&sound, cue);
    for (int chunk = 0; chunk < 3; chunk++) {
        tower_sound_render(&sound, buffer, TOWER_SOUND_CHUNK_SAMPLES);
        for (size_t i = 0; i < TOWER_SOUND_CHUNK_SAMPLES; i++) {
            int32_t sample = buffer[i];
            assert(sample > -20000 && sample < 20000);
            energy += (uint64_t)(sample * sample);
        }
    }
    return energy;
}

static void test_gameplay_cues_stand_above_music(void)
{
    uint64_t music = early_energy(TOWER_SFX_NONE);
    const tower_sfx_t important[] = {
        TOWER_SFX_DROP, TOWER_SFX_LAND, TOWER_SFX_PERFECT,
        TOWER_SFX_MISS, TOWER_SFX_WIN, TOWER_SFX_LOSE,
    };
    for (size_t i = 0; i < sizeof(important) / sizeof(important[0]); i++)
        assert(early_energy(important[i]) >= music * 5U);
}

static void test_music_and_effects_are_distinct(void)
{
    tower_sound_t music;
    tower_sound_t effect;
    int16_t plain[TOWER_SOUND_CHUNK_SAMPLES];
    int16_t mixed[TOWER_SOUND_CHUNK_SAMPLES];
    tower_sound_init(&music);
    tower_sound_init(&effect);
    tower_sound_cue(&effect, TOWER_SFX_START);
    tower_sound_render(&music, plain, TOWER_SOUND_CHUNK_SAMPLES);
    tower_sound_render(&effect, mixed, TOWER_SOUND_CHUNK_SAMPLES);

    int audible = 0;
    int differences = 0;
    for (size_t i = 0; i < TOWER_SOUND_CHUNK_SAMPLES; i++) {
        audible += plain[i] != 0;
        differences += plain[i] != mixed[i];
        assert(plain[i] > -12000 && plain[i] < 12000);
        assert(mixed[i] > -12000 && mixed[i] < 12000);
    }
    assert(audible > 200);
    assert(differences > 200);
}

static void test_important_cue_survives_navigation(void)
{
    tower_sound_t sound;
    int16_t buffer[TOWER_SOUND_CHUNK_SAMPLES];
    tower_sound_init(&sound);
    tower_sound_cue(&sound, TOWER_SFX_WIN);
    tower_sound_cue(&sound, TOWER_SFX_NAV);
    assert(sound.active_cue == TOWER_SFX_WIN);
    for (int i = 0; i < 30; i++)
        tower_sound_render(&sound, buffer, TOWER_SOUND_CHUNK_SAMPLES);
    assert(sound.active_cue == TOWER_SFX_NONE);
    tower_sound_cue(&sound, TOWER_SFX_NAV);
    assert(sound.active_cue == TOWER_SFX_NAV);
}

int main(void)
{
    test_music_and_effects_are_distinct();
    test_important_cue_survives_navigation();
    test_gameplay_cues_stand_above_music();
    puts("tower sound: PASS");
    return 0;
}
