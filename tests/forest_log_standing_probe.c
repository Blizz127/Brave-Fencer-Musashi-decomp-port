/* Synthetic host witness for the standing handler in SC01 room member 77. */
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "psx_types.h"

extern void func_8017F460(u8 *actor);
s32 D_801270CC[1], D_801270D4, D_8018A8F8;
u16 D_80126B66;
s16 D_8018A8FC[3] = {1, 8, 16};

typedef struct {
    u8 to_z[14];
    s16 z;
    u8 to_flags[0x5c - 0x10];
    u16 flags;
    u8 to_variant[0x70 - 0x5e];
    s16 variant;
} Actor;
_Static_assert(offsetof(Actor, z) == 0xe, "z offset");
_Static_assert(offsetof(Actor, flags) == 0x5c, "flags offset");
_Static_assert(offsetof(Actor, variant) == 0x70, "variant offset");

static Actor object;
static int calls;

void func_8017F5BC(u8 *actor)
{
    assert(actor == (u8 *)&object && calls == 0);
    calls = 1;
}

s32 func_8002D4C8(u16 command, u16 arg)
{
    assert(command == 0x495 && arg == 0 && calls == 1);
    calls = 2;
    return -1;
}

static void run(int role, int kind, int phase, int ticks, unsigned flags,
                int actor_z, int player_z, int distance_limit,
                int expected_phase, int expected_ticks, unsigned expected_flags,
                int expected_calls)
{
    Actor expected;
    memset(&object, 0xa5, sizeof(object));
    object.variant = (s16)(role * 256 + kind);
    object.flags = (u16)flags;
    object.z = (s16)actor_z;
    expected = object;
    expected.flags = (u16)expected_flags;
    D_801270CC[0] = phase;
    D_801270D4 = ticks;
    D_80126B66 = (u16)player_z;
    D_8018A8F8 = distance_limit;
    calls = 0;
    func_8017F460((u8 *)&object);
    assert(calls == expected_calls && D_801270CC[0] == expected_phase);
    assert(D_801270D4 == expected_ticks && memcmp(&object, &expected, sizeof(object)) == 0);
}

int main(void)
{
    /* The original observer's standing bridge slots, before any hit. */
    run(0, 4, 0, 0, 0x8000, 2080, 1333, 0, 0, 0, 0x8000, 0);
    run(1, 4, 0, 0, 0x8000, 2080, 1333, 0, 0, 0, 0x8000, 0);
    run(2, 4, 0, 0, 0x8000, 2080, 1333, 0, 0, 0, 0x8000, 0);
    /* A phase-zero hit clears the entire flags field before a threshold test. */
    run(0, 4, 0, 0, 0x8001, 0, 0, 0, 3, 1, 0, 2);
    run(1, 4, 0, 7, 0x8081, 0, 0, 0, 3, 7, 0, 0);
    run(1, 4, 0, 8, 0x0101, 0, 0, 0, 3, 8, 0, 2);
    run(0, 4, 1, 20, 1, 0, 0, 0, 1, 20, 1, 0);
    run(0, 4, -1, 20, 1, 0, 0, 0, -1, 20, 1, 0);
    /* At phase two the hit starts an idle counter but keeps the actor's flags. */
    run(0, 4, 2, 0, 0x8001, 0, 0, 0, 2, 1, 0x8001, 2);
    run(0, 4, 2, -1, 0x8001, 0, 0, 0, 2, -1, 0x8001, 0);
    for (int role = 0; role < 3; ++role) {
        const int threshold = D_8018A8FC[role];
        run(role, 4, 3, threshold-1, 0, 0, 0, 0, 3, threshold-1, 0, 0);
        run(role, 4, 3, threshold, 0, 0, 0, 0, 3, threshold, 0, 2);
        run(role, 4, 3, threshold+1, 0, 0, 0, 0, 3, threshold+1, 0, 2);
    }
    /* Other kinds use strict signed Z distance, with no sound request. */
    run(0, 2, 1, 0, 0x8001, -85, -80, 6, 1, 0, 0x8001, 1);
    run(0, 0, 1, 0, 0x8001, -80, -85, 6, 1, 0, 0x8001, 1);
    run(0, 0, 1, 0, 0x8001, -80, -85, 5, 1, 0, 0x8001, 0);
    run(0, 0, 1, 0, 0x8001, -32768, 32767, 65535, 1, 0, 0x8001, 0);
    run(0, 0, 1, 0, 0x8001, 32767, -32768, 65536, 1, 0, 0x8001, 1);
    return 0;
}
