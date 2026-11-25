#include "scene_countdown.h"
#include "room_descriptor.h"
#include "fade_setup.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

_Static_assert(offsetof(SceneCountdown, phase) == 0x15, "phase offset");
_Static_assert(offsetof(SceneCountdown, timer) == 0x28, "timer offset");

ScenePhaseHandler D_80182B4C[3];
u32 D_80182B64[1];
u8 D_80126B58[2];
static u16 provider_id;
static unsigned provider_calls;
s16 D_800B9A08;
RoomTransferDescriptor D_80078E50;

static SceneCountdown *active;
static unsigned event;
static unsigned calls;
static unsigned scenario;
static s32 fade_result;
static s16 old_mode;
static u16 pending_mode;

enum { STANDARD, CLASSIFIED_CURRENT, PENDING_REJECTED, CLASSIFIED_PENDING, DIRECT, CASE_COUNT };

s32 func_8016EF78(u32 *record, u32 flags)
{
    assert(record == D_80182B64 && flags == 0x01000000u);
    assert(event == 0 && active->phase == 0);
    ++event;
    provider_calls = 1;
    D_80126B58[0] = (u8)provider_id;
    D_80126B58[1] = (u8)(provider_id >> 8);
    return -139;
}

s32 func_801599A4(u8 *actor)
{
    assert(actor == D_80126B58 && provider_calls == 1);
    ++provider_calls;
    return -731;
}

s32 func_80159B3C(u8 *actor)
{
    assert(actor == D_80126B58 && provider_calls == 2);
    ++provider_calls;
    return 317;
}

void func_8002D4C8(u16 command, u16 argument)
{
    assert(active->timer == (u32)-1 && active->phase == 1 && argument == 0);
    assert((event == 1 && command == 0x1c) || (event == 3 && command == 0x1d));
    ++event;
}

void func_8001BFD0(void)
{
    assert(event == 2);
    ++event;
}

void func_800D1E28(void)
{
    assert(event == 4);
    ++event;
}

s32 func_800CFC5C(s32 id)
{
    if (calls == 0) {
        assert(id == old_mode && scenario != STANDARD);
        ++calls;
        return scenario == CLASSIFIED_CURRENT;
    }
    assert(calls == 2 && id == pending_mode);
    ++calls;
    return scenario == CLASSIFIED_PENDING;
}

s32 func_800CFD68(s32 id)
{
    assert(calls == 1 && id == pending_mode);
    ++calls;
    ++pending_mode;
    D_80078E50.room = pending_mode;
    return scenario != PENDING_REJECTED;
}

s32 func_800168C4(s32 lane)
{
    assert(lane == 4 && scenario == DIRECT && calls == 3);
    ++calls;
    return fade_result;
}

s32 func_800167F0(u16 lane)
{
    assert(lane == 4 && scenario != DIRECT);
    assert(calls == scenario);
    ++calls;
    return fade_result;
}

static void run_phase_sequence(SceneCountdown *state)
{
    SceneCountdown expected = *state;
    unsigned tick;

    active = state;
    expected.phase = 1;
    expected.timer = 10;
    assert(func_8017CF1C(state) == 0);
    assert(memcmp(&expected, state, sizeof(expected)) == 0 && event == 1);
    assert(provider_calls == (provider_id == 25 || provider_id == 26 ? 1u : 3u));
    for (tick = 0; tick < 10; ++tick) {
        --expected.timer;
        assert(func_8017CF1C(state) == 0);
        assert(memcmp(&expected, state, sizeof(expected)) == 0 && event == 1);
    }
    ++expected.phase;
    --expected.timer;
    assert(func_8017CF1C(state) == 0);
    assert(memcmp(&expected, state, sizeof(expected)) == 0 && event == 5);
    assert(func_8017CF1C(state) == ((u16)fade_result != 0));
    assert(memcmp(&expected, state, sizeof(expected)) == 0 && event == 5);
    assert(calls == (scenario == DIRECT ? 4u : scenario + 1));
}

static s32 substitute(SceneCountdown *state)
{
    assert(state == active);
    ++state->timer;
    return fade_result;
}

int main(void)
{
    static SceneCountdown state;
    unsigned seed;
    memset(&state, 0, sizeof(state));
    active = &state;
    D_80182B4C[0] = substitute;
    for (seed = 0; seed < 2; ++seed) {
        fade_result = seed ? -431 : 729;
        assert(func_8017CF1C(&state) == fade_result);
        assert(state.timer == seed + 1);
    }
    D_80182B4C[0] = func_8017CE40;
    D_80182B4C[1] = func_8017CF58;
    D_80182B4C[2] = func_8017CEFC;

    for (scenario = STANDARD; scenario < CASE_COUNT; ++scenario) {
        for (seed = 0; seed < 256; ++seed) {
            memset(&state, (int)seed, sizeof(state));
            state.phase = 0;
            provider_id = (u16)(seed + 11);
            event = calls = 0;
            old_mode = scenario == STANDARD ? 0x3005 : (s16)(-1000 - (int)seed);
            pending_mode = (u16)(0x8123u + seed);
            D_800B9A08 = old_mode;
            D_80078E50.room = pending_mode;
            fade_result = (s32)((seed << 16) | (seed & 1));
            run_phase_sequence(&state);
        }
    }
    return 0;
}
