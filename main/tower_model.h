// Pure Tower Bloxx game rules. No ESP-IDF or LVGL dependency; host tests own the rules.
#pragma once

#include <stdbool.h>
#include <stdint.h>

// All coordinates are local to the 226-pixel-wide playfield, not LCD GPIO or panel constants.
#define TOWER_CENTER_X 113
#define TOWER_FLOOR_WIDTH 48
#define TOWER_FLOOR_HEIGHT 48
#define TOWER_ACTIVE_Y 14
#define TOWER_BASE_Y 128
#define TOWER_FIELD_HEIGHT 233
#define TOWER_MAX_FLOORS 99
#define TOWER_MAX_MISSES 3
#define TOWER_CITY_CELLS 16

typedef enum {
    TOWER_MENU,       // Choose city or quick mode.
    TOWER_SWING,      // Crane is moving and accepts a release action.
    TOWER_FALLING,    // Released floor is moving toward the stack.
    TOWER_RESULT,     // Construction ended or a city tower was completed.
    TOWER_CITY,       // Place a completed tower or choose the next tier.
} tower_phase_t;

typedef enum {
    TOWER_MODE_CITY,
    TOWER_MODE_QUICK,
} tower_mode_t;

typedef enum {
    TOWER_NONE,
    TOWER_LANDED,
    TOWER_MISSED,
    TOWER_COMPLETE,
    TOWER_GAME_OVER,
} tower_event_t;

typedef enum {
    TOWER_GRADE_MISS,
    TOWER_GRADE_RISKY,
    TOWER_GRADE_OKAY,
    TOWER_GRADE_GREAT,
    TOWER_GRADE_PERFECT,
} tower_grade_t;

typedef struct {
    uint8_t type;         // 0 means empty; otherwise 10, 20, 30, or 40 floors.
    uint32_t population;  // Residents earned while constructing this tower.
} tower_city_cell_t;

typedef struct {
    tower_phase_t phase;                         // Current screen/gameplay phase.
    tower_mode_t mode;                           // City progression or endless quick game.
    uint8_t target;                              // City target floors; 0 in quick mode.
    uint8_t floor_count;                         // Number of successfully stacked floors.
    int16_t floor_x[TOWER_MAX_FLOORS];           // Floor centers, oldest to newest.
    int16_t active_x;                            // Current crane/falling floor center.
    int16_t fall_y;                              // Falling floor top within playfield.
    int32_t fall_speed_px_s;                     // Vertical velocity, integer px/s.
    uint32_t swing_ms;                           // Time within current crane cycle.
    uint32_t population;                         // Residents in the active tower.
    uint32_t city_population;                    // Residents in placed city towers.
    uint8_t misses;                              // Failed drops, capped at three.
    uint8_t streak;                              // Consecutive perfect placements.
    tower_grade_t last_grade;                    // Last placement feedback grade.
    uint16_t last_gain;                          // Residents added by the last drop.
    uint8_t pending_type;                        // Completed tower awaiting city placement.
    tower_city_cell_t city[TOWER_CITY_CELLS];    // Four-by-four city grid.
} tower_model_t;

// Reset all state, including city progression. Safe in any phase; no allocation or blocking.
void tower_model_init(tower_model_t *model);

// Start a tower without clearing the existing city. Returns false for an unavailable tier.
bool tower_model_start(tower_model_t *model, tower_mode_t mode, uint8_t target);

// Advance crane/fall by elapsed milliseconds (capped internally to avoid large jumps).
// Returns a landing/completion event or TOWER_NONE; no I/O or allocation.
tower_event_t tower_model_tick(tower_model_t *model, uint32_t elapsed_ms);

// Freeze the crane and begin falling. Returns false outside TOWER_SWING.
bool tower_model_release(tower_model_t *model);

// Resolve a released floor at x when it reaches the stack. Exposed for deterministic tests.
// Returns the placement event; returns TOWER_NONE outside TOWER_FALLING.
tower_event_t tower_model_land(tower_model_t *model, int16_t x);

// Camera offset keeps the top of tall towers visible within the 240x320 game screen.
int16_t tower_model_camera_offset(const tower_model_t *model);

// City actions. They do not allocate, block, or touch hardware.
bool tower_model_open_city(tower_model_t *model);
bool tower_model_can_place(const tower_model_t *model, uint8_t index, uint8_t type);
bool tower_model_place(tower_model_t *model, uint8_t index);
bool tower_model_can_build(const tower_model_t *model, uint8_t type);

// Return to the mode menu while preserving the city grid for the current session.
void tower_model_show_menu(tower_model_t *model);
