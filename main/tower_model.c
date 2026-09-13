#include "tower_model.h"

#include <string.h>

// A triangular crane path uses only integer arithmetic on the ESP32-C3.
// Rising floors make both amplitude and frequency increase, up to readable limits.
static uint32_t swing_period_ms(const tower_model_t *model)
{
    uint32_t reduction = (uint32_t)model->floor_count * 20U;
    return reduction >= 900U ? 900U : 1800U - reduction;
}

static int16_t swing_amplitude(const tower_model_t *model)
{
    int amplitude = 38 + (int)model->floor_count;
    return (int16_t)(amplitude > 55 ? 55 : amplitude);
}

// Convert the current phase to a center-first, right-moving triangle wave.
static int16_t swing_x(const tower_model_t *model)
{
    uint32_t period = swing_period_ms(model);
    uint32_t quarter = period / 4U;
    uint32_t phase = model->swing_ms % period;
    int32_t amplitude = swing_amplitude(model);
    int32_t offset;
    if (phase < quarter) {
        offset = amplitude * (int32_t)phase / (int32_t)quarter;
    } else if (phase < quarter * 3U) {
        offset = amplitude - amplitude * (int32_t)(phase - quarter) / (int32_t)quarter;
    } else {
        offset = -amplitude + amplitude * (int32_t)(phase - quarter * 3U) /
                                 (int32_t)quarter;
    }
    return (int16_t)(TOWER_CENTER_X + offset);
}

// Check a type's unlock prerequisite; map placement is checked separately.
static bool tier_unlocked(const tower_model_t *model, uint8_t type)
{
    if (type == 10) return true;
    uint8_t prerequisite = (uint8_t)(type - 10);
    if (type != 20 && type != 30 && type != 40) return false;
    for (uint8_t i = 0; i < TOWER_CITY_CELLS; i++) {
        if (model->city[i].type == prerequisite) return true;
    }
    return false;
}

void tower_model_init(tower_model_t *model)
{
    if (!model) return;
    memset(model, 0, sizeof(*model));
    model->phase = TOWER_MENU;
    model->active_x = TOWER_CENTER_X;
}

bool tower_model_start(tower_model_t *model, tower_mode_t mode, uint8_t target)
{
    if (!model) return false;
    if (mode == TOWER_MODE_CITY) {
        if (!tower_model_can_build(model, target)) return false;
    } else if (mode != TOWER_MODE_QUICK || target != 0) {
        return false;
    }
    model->mode = mode;
    model->target = target;
    model->phase = TOWER_SWING;
    model->floor_count = 0;
    model->active_x = TOWER_CENTER_X;
    model->fall_y = TOWER_ACTIVE_Y;
    model->fall_speed_px_s = 0;
    model->swing_ms = 0;
    model->population = 0;
    model->misses = 0;
    model->streak = 0;
    model->last_grade = TOWER_GRADE_MISS;
    model->last_gain = 0;
    model->pending_type = 0;
    memset(model->floor_x, 0, sizeof(model->floor_x));
    return true;
}

int16_t tower_model_camera_offset(const tower_model_t *model)
{
    if (!model) return 0;
    // A whole square module scrolls down after each landing, keeping room for
    // the next hanging module above the tower on the 240x320 display.
    return (int16_t)(model->floor_count * TOWER_FLOOR_HEIGHT);
}

tower_event_t tower_model_tick(tower_model_t *model, uint32_t elapsed_ms)
{
    if (!model) return TOWER_NONE;
    // A suspended firmware task must not tunnel a falling floor through the stack.
    if (elapsed_ms > 100U) elapsed_ms = 100U;
    if (model->phase == TOWER_SWING) {
        model->swing_ms += elapsed_ms;
        model->active_x = swing_x(model);
    } else if (model->phase == TOWER_FALLING) {
        model->fall_speed_px_s += (int32_t)(650U * elapsed_ms / 1000U);
        model->fall_y += (int16_t)(model->fall_speed_px_s * (int32_t)elapsed_ms / 1000);
        int16_t landing_y = (int16_t)(TOWER_BASE_Y -
            (model->floor_count + 1) * TOWER_FLOOR_HEIGHT +
            tower_model_camera_offset(model));
        if (model->fall_y >= landing_y) {
            model->fall_y = landing_y;
            return tower_model_land(model, model->active_x);
        }
    }
    return TOWER_NONE;
}

bool tower_model_release(tower_model_t *model)
{
    if (!model || model->phase != TOWER_SWING) return false;
    model->phase = TOWER_FALLING;
    model->fall_y = TOWER_ACTIVE_Y;
    model->fall_speed_px_s = 0;
    return true;
}

tower_event_t tower_model_land(tower_model_t *model, int16_t x)
{
    if (!model || model->phase != TOWER_FALLING) return TOWER_NONE;
    int16_t target_x = model->floor_count
        ? model->floor_x[model->floor_count - 1] : TOWER_CENTER_X;
    int32_t offset = x >= target_x ? x - target_x : target_x - x;
    int32_t overlap = TOWER_FLOOR_WIDTH - offset;
    model->last_gain = 0;
    model->active_x = x;
    if (overlap * 100 < TOWER_FLOOR_WIDTH * 38) {
        model->last_grade = TOWER_GRADE_MISS;
        model->misses++;
        model->streak = 0;
        if (model->misses >= TOWER_MAX_MISSES) {
            model->phase = TOWER_RESULT;
            return TOWER_GAME_OVER;
        }
        model->phase = TOWER_SWING;
        model->swing_ms = 0;
        return TOWER_MISSED;
    }

    if (offset <= 5) model->last_grade = TOWER_GRADE_PERFECT;
    else if (offset <= 16) model->last_grade = TOWER_GRADE_GREAT;
    else if (overlap * 100 >= TOWER_FLOOR_WIDTH * 60)
        model->last_grade = TOWER_GRADE_OKAY;
    else model->last_grade = TOWER_GRADE_RISKY;

    uint32_t base = 26U + (uint32_t)model->streak * 4U;
    uint32_t gained = base * (uint32_t)overlap / TOWER_FLOOR_WIDTH;
    if (model->last_grade == TOWER_GRADE_PERFECT) {
        gained = gained * 3U / 2U;
        if (model->streak < 10) model->streak++;
    } else {
        model->streak = 0;
    }
    if (gained == 0) gained = 1;
    model->last_gain = (uint16_t)gained;
    model->population += gained;
    model->floor_x[model->floor_count++] = x;
    model->swing_ms = 0;
    model->active_x = TOWER_CENTER_X;

    if ((model->mode == TOWER_MODE_CITY && model->floor_count >= model->target) ||
        model->floor_count >= TOWER_MAX_FLOORS) {
        model->phase = TOWER_RESULT;
        if (model->mode == TOWER_MODE_CITY) model->pending_type = model->target;
        return TOWER_COMPLETE;
    }
    model->phase = TOWER_SWING;
    return TOWER_LANDED;
}

bool tower_model_open_city(tower_model_t *model)
{
    if (!model || model->phase != TOWER_RESULT || !model->pending_type) return false;
    model->phase = TOWER_CITY;
    return true;
}

bool tower_model_can_place(const tower_model_t *model, uint8_t index, uint8_t type)
{
    if (!model || index >= TOWER_CITY_CELLS || model->city[index].type) return false;
    if (type == 10) return true;
    if (type != 20 && type != 30 && type != 40) return false;
    uint8_t row = index / 4U;
    uint8_t col = index % 4U;
    uint8_t neighbors[4];
    uint8_t count = 0;
    if (row > 0) neighbors[count++] = (uint8_t)(index - 4U);
    if (row < 3) neighbors[count++] = (uint8_t)(index + 4U);
    if (col > 0) neighbors[count++] = (uint8_t)(index - 1U);
    if (col < 3) neighbors[count++] = (uint8_t)(index + 1U);
    bool has_10 = false, has_20 = false, has_30 = false;
    for (uint8_t i = 0; i < count; i++) {
        uint8_t neighbor_type = model->city[neighbors[i]].type;
        has_10 |= neighbor_type == 10;
        has_20 |= neighbor_type == 20;
        has_30 |= neighbor_type == 30;
    }
    if (type == 20) return has_10;
    if (type == 30) return has_10 && has_20;
    return has_10 && has_20 && has_30;
}

bool tower_model_place(tower_model_t *model, uint8_t index)
{
    if (!model || model->phase != TOWER_CITY || !model->pending_type ||
        !tower_model_can_place(model, index, model->pending_type)) return false;
    model->city[index].type = model->pending_type;
    model->city[index].population = model->population;
    model->city_population += model->population;
    model->pending_type = 0;
    return true;
}

bool tower_model_can_build(const tower_model_t *model, uint8_t type)
{
    if (!model || !tier_unlocked(model, type)) return false;
    for (uint8_t i = 0; i < TOWER_CITY_CELLS; i++) {
        if (tower_model_can_place(model, i, type)) return true;
    }
    return false;
}

void tower_model_show_menu(tower_model_t *model)
{
    if (!model) return;
    model->phase = TOWER_MENU;
    model->pending_type = 0;
}
