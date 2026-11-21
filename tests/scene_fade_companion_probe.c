#include "fade_setup.h"
#include "scene_countdown.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

ScenePhaseHandler D_80182B38[2];
u32 D_80182B58[1];
u8 D_80126B58[2];
static SceneCountdown record;
static u16 actor_id;
static s32 fade_result;
static unsigned calls, side_effects;

s32 func_8016EF78(u32 *descriptor, u32 flags)
{
    assert(calls++ == 0 && descriptor == D_80182B58 && flags == (1u << 24));
    D_80126B58[0] = (u8)actor_id;
    D_80126B58[1] = (u8)(actor_id >> 8);
    return -15;
}

s32 func_801599A4(u8 *actor)
{
    assert(calls++ == 1 && actor == D_80126B58);
    return 42;
}

s32 func_80159B3C(u8 *actor)
{
    assert(calls++ == 2 && actor == D_80126B58);
    return fade_result;
}

void func_800167B8(s32 phase)
{
    assert(phase == 0);
    assert(calls == (actor_id == 25 || actor_id == 26 ? 1u : 3u));
    ++calls;
    if (side_effects) {
        record.phase = (u8)actor_id;
        ++record.timer;
    }
}

s32 func_800167F0(u16 phase)
{
    assert(phase == 0);
    ++calls;
    return fade_result;
}

static s32 alternate(SceneCountdown *state)
{
    assert(state == &record);
    ++calls;
    return fade_result;
}

int main(void)
{
    unsigned seed;
    D_80182B38[0] = func_8017CDD8;
    D_80182B38[1] = func_8017CE1C;
    for (seed = 0; seed < 4096; ++seed) {
        SceneCountdown expected;
        memset(&record, (int)(u8)seed, sizeof(record));
        record.phase = 0;
        record.timer = seed * 99991u;
        actor_id = (u16)(seed & 0x1ff);
        side_effects = (seed >> 9) & 1;
        fade_result = (seed & 1) ? (s32)(seed | (seed << 16)) : (s32)(seed << 16);
        expected = record;
        if (side_effects) {
            expected.phase = (u8)actor_id;
            ++expected.timer;
        }
        ++expected.phase;
        calls = 0;
        assert(func_8017CD9C(&record) == 0);
        assert(calls == (actor_id == 25 || actor_id == 26 ? 2u : 4u));
        assert(memcmp(&expected, &record, sizeof(record)) == 0);
        calls = 0;
        record.phase = 1;
        expected = record;
        assert(func_8017CD9C(&record) == ((u16)fade_result != 0));
        assert(calls == 1 && memcmp(&expected, &record, sizeof(record)) == 0);
    }
    D_80182B38[1] = alternate;
    calls = 0;
    fade_result = INT_MIN;
    assert(func_8017CD9C(&record) == INT_MIN && calls == 1);
    return 0;
}
