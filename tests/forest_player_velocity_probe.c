#include <assert.h>
#include <stddef.h>
#include "forest_action.h"

int main(void)
{
    ForestActionPlayer actor = {0};
    const u32 flags = 0x410u;

    _Static_assert(offsetof(ForestActionPlayer, position) == 4, "position offset");
    _Static_assert(offsetof(ForestActionPlayer, velocity) == 16, "velocity offset");
    _Static_assert(offsetof(ForestActionPlayer, flags) == 68, "flag offset");
    actor.position[0] = 0x7fffffffu;
    actor.position[1] = 0x80000000u;
    actor.position[2] = 0xfffffff0u;
    actor.flags = flags;
    assert(func_801473EC(&actor) == 0 && actor.flags == flags);
    for (unsigned int axis = 0; axis < 3; ++axis) {
        u32 before[3];
        for (unsigned int i = 0; i < 3; ++i) before[i] = actor.position[i];
        actor.velocity[axis] = axis == 1 ? 0xffffffffu : 0x20u;
        assert((u32)func_801473EC(&actor) == (flags | 0x80000000u));
        for (unsigned int i = 0; i < 3; ++i)
            assert(actor.position[i] == before[i] + actor.velocity[i]);
        assert(actor.flags == (flags | 0x80000000u));
        actor.velocity[axis] = 0;
    }
    assert(func_801473EC(&actor) == 0 && actor.flags == (flags | 0x80000000u));
    return 0;
}
