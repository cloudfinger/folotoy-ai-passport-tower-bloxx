#include "tower_model.h"

#include <assert.h>
#include <stdio.h>

_Static_assert(TOWER_FLOOR_WIDTH == TOWER_FLOOR_HEIGHT,
               "Building modules must have a square front face");

static void test_square_module_has_drop_clearance(void)
{
    tower_model_t game;
    tower_model_init(&game);
    assert(tower_model_start(&game, TOWER_MODE_QUICK, 0));
    assert(TOWER_FLOOR_WIDTH >= 48);
    assert(TOWER_BASE_Y + 2 * TOWER_FLOOR_HEIGHT <= TOWER_FIELD_HEIGHT);
    assert(tower_model_release(&game));
    assert(tower_model_land(&game, TOWER_CENTER_X) == TOWER_LANDED);

    int next_landing = TOWER_BASE_Y - 2 * TOWER_FLOOR_HEIGHT +
                       tower_model_camera_offset(&game);
    assert(next_landing > TOWER_ACTIVE_Y + TOWER_FLOOR_HEIGHT);
}

static void test_crane_and_scoring(void)
{
    tower_model_t game;
    tower_model_init(&game);
    assert(game.phase == TOWER_MENU);
    assert(tower_model_start(&game, TOWER_MODE_CITY, 10));
    assert(game.active_x == TOWER_CENTER_X);
    tower_model_tick(&game, 450);
    assert(game.active_x > TOWER_CENTER_X);

    assert(tower_model_release(&game));
    assert(tower_model_land(&game, TOWER_CENTER_X) == TOWER_LANDED);
    assert(game.floor_count == 1);
    const uint32_t perfect_population = game.population;

    assert(tower_model_release(&game));
    assert(tower_model_land(&game, TOWER_CENTER_X + 25) == TOWER_LANDED);
    assert(game.floor_count == 2);
    assert(game.population > perfect_population);
    assert(game.last_grade != TOWER_GRADE_PERFECT);
}

static void test_three_misses_end_construction(void)
{
    tower_model_t game;
    tower_model_init(&game);
    assert(tower_model_start(&game, TOWER_MODE_QUICK, 0));
    for (int i = 0; i < TOWER_MAX_MISSES; i++) {
        assert(tower_model_release(&game));
        tower_event_t event = tower_model_land(&game, TOWER_CENTER_X + TOWER_FLOOR_WIDTH);
        assert(event == (i == TOWER_MAX_MISSES - 1 ? TOWER_GAME_OVER : TOWER_MISSED));
    }
    assert(game.phase == TOWER_RESULT);
    assert(game.floor_count == 0);
}

static void test_animated_fall_reaches_the_stack(void)
{
    tower_model_t game;
    tower_model_init(&game);
    assert(tower_model_start(&game, TOWER_MODE_QUICK, 0));
    assert(tower_model_release(&game));
    tower_event_t event = TOWER_NONE;
    for (int frame = 0; frame < 50 && event == TOWER_NONE; frame++) {
        event = tower_model_tick(&game, 30);
    }
    assert(event == TOWER_LANDED);
    assert(game.floor_count == 1);
    assert(game.phase == TOWER_SWING);
}

static void test_city_completion_and_placement(void)
{
    tower_model_t game;
    tower_model_init(&game);
    assert(tower_model_start(&game, TOWER_MODE_CITY, 10));
    for (int i = 0; i < 10; i++) {
        assert(tower_model_release(&game));
        tower_event_t event = tower_model_land(&game, TOWER_CENTER_X);
        assert(event == (i == 9 ? TOWER_COMPLETE : TOWER_LANDED));
    }
    assert(game.phase == TOWER_RESULT);
    assert(game.pending_type == 10);
    assert(tower_model_open_city(&game));
    assert(tower_model_place(&game, 5));
    assert(game.city[5].type == 10);
    assert(game.city_population == game.population);
    assert(tower_model_can_build(&game, 20));
    assert(!tower_model_can_build(&game, 30));
}

static void test_neighbor_rules(void)
{
    tower_model_t game;
    tower_model_init(&game);
    game.city[5].type = 10;
    game.city[6].type = 20;
    game.city[9].type = 30;
    game.city[11].type = 10;
    assert(tower_model_can_place(&game, 4, 20));
    assert(!tower_model_can_place(&game, 0, 20));
    assert(tower_model_can_place(&game, 10, 40));
    assert(!tower_model_can_place(&game, 5, 10));
}

int main(void)
{
    test_square_module_has_drop_clearance();
    test_crane_and_scoring();
    test_three_misses_end_construction();
    test_animated_fall_reaches_the_stack();
    test_city_completion_and_placement();
    test_neighbor_rules();
    puts("tower model: PASS");
    return 0;
}
