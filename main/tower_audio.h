// Nonblocking game-audio control. PCM I/O belongs only to the audio worker.
#pragma once

#include "tower_sound.h"

#include <stdbool.h>

bool tower_audio_start(void);
void tower_audio_play(tower_sfx_t cue);
void tower_audio_stop(void);
