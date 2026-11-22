#include "mine_actor_motion.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

_Static_assert(offsetof(MineActorMotion, position) == 4, "position offset");
_Static_assert(offsetof(MineActorMotion, velocity) == 0x10, "velocity offset");
_Static_assert(offsetof(MineActorMotion, vertical_step) == 0x34, "vertical step offset");
_Static_assert(offsetof(MineActorMotion, flags) == 0x44, "flags offset");

int main(void)
{
    const u32 limits[] = {0u, 1u, 0xffffffffu, 0x7fffffffu, 0x80000000u};
    MineActorMotion state, expected;

    for (unsigned mask = 0; mask < 8; ++mask) {
        for (unsigned i = 0; i < sizeof limits / sizeof *limits; ++i) {
            memset(&state, 0x63, sizeof state);
            state.flags = limits[i];
            for (unsigned axis = 0; axis < 3; ++axis) {
                state.position[axis] = limits[(i + axis) % 5];
                state.velocity[axis] = mask & (1u << axis) ? limits[1 + (i + axis) % 4] : 0;
            }
            expected = state;
            for (unsigned axis = 0; axis < 3; ++axis)
                expected.position[axis] += expected.velocity[axis];
            if (mask) expected.flags |= 0x80000000u;
            assert(func_801473EC(&state) == (mask ? expected.flags : 0));
            assert(!memcmp(&state, &expected, sizeof state));

            state.vertical_step = limits[i];
            expected = state;
            expected.position[1] += state.vertical_step;
            assert(func_80147460(&state) == expected.position[1]);
            assert(!memcmp(&state, &expected, sizeof state));
        }
    }
    return 0;
}
