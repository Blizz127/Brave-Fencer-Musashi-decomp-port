#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "mine_frame_state.h"

_Static_assert(offsetof(MineFrameMode, flag) == 9, "flag byte offset");

MineFrameState D_8011F750;
MineFrameMode D_8011F7A8;
void (*D_80181CA0[255])(MineFrameState *);
void (*D_801828E8[256])(MineFrameMode *);
u8 D_800B9A13;
u32 D_80126B9C;

static unsigned timer_calls, optional_calls, mode_calls;
static unsigned selected, expected_optional, expected_flag;
static u32 next_tick, next_mask;
static u8 next_gate, next_mode;

static void optional_first(MineFrameState *state)
{
    assert(state == &D_8011F750);
    assert(selected == 0 && state->state == expected_optional);
    selected = 1;
    state->state = 0;
    ++optional_calls;
}

static void optional_last(MineFrameState *state)
{
    optional_first(state);
    selected = 2;
}

u32 func_80029504(void)
{
    assert(timer_calls == mode_calls);
    ++timer_calls;
    D_8011F7A8.mode = next_mode;
    D_800B9A13 = next_gate;
    D_80126B9C = next_mask;
    return next_tick;
}

static void mode_even(MineFrameMode *state)
{
    assert(state == &D_8011F7A8);
    assert(timer_calls == mode_calls + 1 && state->mode == next_mode);
    assert(state->flag == expected_flag && selected == 0);
    selected = 1;
    ++mode_calls;
    state->flag = 0x59;
}

static void mode_odd(MineFrameMode *state)
{
    mode_even(state);
    selected = 2;
}

int main(void)
{
    const u32 ticks[] = {0, 9, 10, 11, 1208, 1209, 1210, 1211, ~0u};
    const u32 masks[] = {0, 0x40, 0x80, 0x80000000u, 0x80000080u};
    const u8 gates[] = {0, 1, 255};
    const u8 modes[] = {0, 1, 2, 255};
    unsigned i, j, k, m;

    memset(&D_8011F750, 0x61, sizeof(D_8011F750));
    D_8011F750.state = 0;
    func_801627E8();
    assert(optional_calls == 0 && selected == 0);
    for (i = 1; i < 256; ++i) {
        D_80181CA0[i - 1] = i % 2 ? optional_first : optional_last;
        expected_optional = i;
        D_8011F750.state = (u8)i;
        selected = 0;
        func_801627E8();
        assert(optional_calls == i && selected == (i % 2 ? 1u : 2u));
        assert(D_8011F750.state == 0);
        func_801627E8();
        assert(optional_calls == i);
    }
    for (i = 0; i < sizeof(D_8011F750.reserved); ++i)
        assert(D_8011F750.reserved[i] == 0x61);

    memset(&D_8011F7A8, 0x2b, sizeof(D_8011F7A8));
    for (m = 0; m < sizeof(modes) / sizeof(modes[0]); ++m) {
        next_mode = modes[m];
        D_801828E8[next_mode] = next_mode % 2 ? mode_odd : mode_even;
        for (i = 0; i < sizeof(ticks) / sizeof(ticks[0]); ++i)
            for (j = 0; j < sizeof(gates) / sizeof(gates[0]); ++j)
                for (k = 0; k < sizeof(masks) / sizeof(masks[0]); ++k) {
                    next_tick = ticks[i];
                    next_gate = gates[j];
                    next_mask = masks[k];
                    expected_flag = (next_gate == 0 && !(next_mask & 128)) ||
                                    next_tick < 10 || next_tick >= 1210;
                    D_8011F7A8.mode = (u8)~next_mode;
                    D_8011F7A8.flag = 0xc3;
                    D_800B9A13 = !next_gate;
                    D_80126B9C = ~next_mask;
                    selected = 0;
                    func_801754A8();
                    assert(selected == (next_mode % 2 ? 2u : 1u));
                    assert(D_8011F7A8.flag == 0x59);
                }
    }
    assert(mode_calls == 540 && timer_calls == 540);
    for (i = 0; i < sizeof(D_8011F7A8.reserved); ++i)
        assert(D_8011F7A8.reserved[i] == 0x2b);
    return 0;
}
