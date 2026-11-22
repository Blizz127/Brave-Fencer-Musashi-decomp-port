#include "forest_tutorial_soldier.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern void func_8017E1D4(ForestTutorialScript *script);
extern void func_8017E460(ForestTutorialScript *script);

u8 D_8018A868[1];
s32 D_801270D0[1];
static ForestTutorialScript *current_script;
static int requests;
static int unexpected_request;
static int simulate_full_pool;
static int invoke_other_stage;
static int other_calls;

s32 func_8012C51C(void *placement, u8 *owner)
{
    ++requests;
    if (placement != D_8018A868 || owner != NULL || D_801270D0[0] != 0) {
        unexpected_request = 1;
    }
    if (invoke_other_stage) {
        current_script->stage = 9;
        current_script->countdown = 80;
    }
    return simulate_full_pool ? 0 : 1;
}

static void other_stage(ForestTutorialScript *script)
{
    if (script == current_script) {
        ++other_calls;
    }
}

ForestTutorialStage D_8018A760[12] = {
    [0] = other_stage,
    [5] = func_8017E460,
    [10] = other_stage
};

static int run_case(u32 delay, u8 stage, s32 already_spawned, int full, int mutate,
                    u32 expected_delay, u8 expected_stage, s32 expected_latch,
                    int expected_calls, int expected_others)
{
    ForestTutorialScript actual;
    ForestTutorialScript expected;

    memset(&actual, 0xA5, sizeof(actual));
    actual.countdown = delay;
    actual.stage = stage;
    expected = actual;
    expected.countdown = expected_delay;
    expected.stage = expected_stage;
    current_script = &actual;
    D_801270D0[0] = already_spawned;
    simulate_full_pool = full;
    invoke_other_stage = mutate;
    requests = unexpected_request = other_calls = 0;
    func_8017E1D4(&actual);
    return memcmp(&actual, &expected, sizeof(actual)) == 0 &&
           D_801270D0[0] == expected_latch && requests == expected_calls &&
           !unexpected_request && other_calls == expected_others;
}

int main(void)
{
    if (offsetof(ForestTutorialScript, countdown) != 0x204 ||
        offsetof(ForestTutorialScript, stage) != 0x214) {
        return 1;
    }
    if (!run_case(1, 5, 0, 0, 0, 0, 5, 0, 0, 0) ||
        !run_case((u32)-1, 5, 0, 0, 0, (u32)-2, 5, 0, 0, 0) ||
        !run_case(0, 5, 0, 0, 0, 64, 6, 1, 1, 0) ||
        !run_case(0, 5, 0, 1, 0, 64, 6, 1, 1, 0) ||
        !run_case(0, 5, -1, 0, 0, 64, 6, -1, 0, 0) ||
        !run_case(0, 5, 0, 0, 1, 64, 10, 1, 1, 0) ||
        !run_case(10, 0, 0, 0, 0, 10, 0, 0, 0, 1) ||
        !run_case(0, 10, 0, 0, 0, 0, 10, 0, 0, 1)) {
        return 2;
    }
    puts("Eight forest tutorial dispatch and soldier request cases passed.");
    return 0;
}
