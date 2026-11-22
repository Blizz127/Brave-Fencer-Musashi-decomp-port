#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "mine_award_early.h"

static u8 player[64], separate_vector[8];
static u8 *target;
static s32 move_result, planar_result;
static unsigned calls;

s32 func_80171D78(u8 *actor, u8 *vector)
{
    assert(calls++ == 0 && actor == player && vector == target);
    actor[7] = 41;
    vector[0] = 73;
    return move_result;
}

s32 func_8017250C(u8 *actor, u8 *vector)
{
    assert(calls++ == 1 && actor == player && vector == target);
    assert(vector[0] == 73);
    assert(actor[7] == (vector == actor + 7 ? 73 : 41));
    vector[2] = 19;
    return planar_result;
}

void func_80146D90(u8 *actor)
{
    assert(calls++ == 2 && actor == player && target[2] == 19);
    actor[25] = 57;
}

int main(void)
{
    static const s32 results[] = {0, 1, -1, INT32_MIN, INT32_MAX, 43513};
    u8 *targets[] = {separate_vector, player, player + 7, player + 23};
    for (unsigned t = 0; t < sizeof targets / sizeof targets[0]; ++t) {
        target = targets[t];
        for (unsigned m = 0; m < sizeof results / sizeof results[0]; ++m) {
            for (unsigned p = 0; p < sizeof results / sizeof results[0]; ++p) {
                u8 expected_player[sizeof player] = {0};
                u8 expected_separate[sizeof separate_vector] = {0};
                u8 *expected_target = t ? expected_player + (target - player) : expected_separate;
                memset(player, 0, sizeof player);
                memset(separate_vector, 0, sizeof separate_vector);
                calls = 0;
                move_result = results[m];
                planar_result = results[p];

                expected_player[7] = 41;
                expected_target[0] = 73;
                expected_target[2] = 19;
                if (planar_result)
                    expected_player[25] = 57;
                assert(func_80171CC4(player, target) == move_result);
                assert(calls == (planar_result ? 3u : 2u));
                assert(!memcmp(expected_player, player, sizeof player));
                assert(!memcmp(expected_separate, separate_vector, sizeof separate_vector));
            }
        }
    }
    return 0;
}
