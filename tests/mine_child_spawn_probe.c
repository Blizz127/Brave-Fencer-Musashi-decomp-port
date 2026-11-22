#include "mine_child_spawn.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

_Static_assert(offsetof(MineEmitterActor, timer) == 0x1c, "timer");
_Static_assert(offsetof(MineEmitterActor, subtype) == 0x70, "subtype");
_Static_assert(offsetof(MineEmitterActor, distance) == 0xe0, "distance");
_Static_assert(offsetof(MineEmitterActor, index) == 0xfe, "sequence index");
_Static_assert(offsetof(MineEmitterActor, range) == 0x100, "range");
_Static_assert(sizeof(MineEmitterStep) == 2, "step pair");

MineEmitterStep D_8019B594[] = {
    {0x12, 9}, {0x04, 46}, {0xff, 0xb2}, {0x40, 122}, {0x08, 18}
};
static MineEmitterActor actor;
static unsigned calls, order[4];
static int expired, sample, reset_on_refresh, switch_on_ready, switch_on_sample;
static int change_on_spawn, allocate;
static s32 seen_subtype, seen_timer;
static s16 seen_index;

static void record(unsigned event)
{
    assert(calls < 4);
    order[calls++] = event;
}

void func_8017E794(MineEmitterActor *parent)
{
    assert(parent == &actor);
    record(1);
    if (reset_on_refresh)
        parent->index = 1;
}

s32 func_8012BEE8(MineEmitterActor *parent)
{
    assert(parent == &actor);
    record(2);
    if (switch_on_ready)
        parent->index = 2;
    return expired;
}

s32 func_8005C49C(void)
{
    record(3);
    if (switch_on_sample) {
        actor.index = 4;
        actor.range = -255;
        actor.distance = -32771;
    }
    return sample;
}

s32 func_8012C658(s16 type, s16 subtype, void *parent_address)
{
    MineEmitterActor *parent = parent_address;
    assert(type == 0x1a9 && parent == &actor);
    record(4);
    seen_subtype = subtype;
    seen_timer = parent->timer;
    seen_index = parent->index;
    if (change_on_spawn) {
        parent->timer = -7913;
        parent->index = 32767;
    }
    return allocate ? -2146435072 : 0;
}

static void setup(void)
{
    memset(&actor, 0x37, sizeof(actor));
    memset(order, 0, sizeof(order));
    actor.subtype = 3;
    actor.range = 20;
    actor.distance = 90;
    actor.timer = -19;
    actor.index = 0;
    calls = 0;
    expired = 1;
    sample = reset_on_refresh = switch_on_ready = switch_on_sample = change_on_spawn = 0;
    allocate = 1;
    seen_subtype = -1;
}

static void expect_order(const unsigned *expected, unsigned count)
{
    assert(calls == count);
    assert(memcmp(expected, order, count * sizeof(*expected)) == 0);
}

int main(void)
{
    static const struct { s16 range; s32 distance; int spawn; } bounds[] = {
        {-256, 335, 0}, {-255, 335, 1}, {255, 335, 1}, {256, 335, 0},
        {0, 336, 0}, {0, INT_MAX, 0}, {0, INT_MIN, 1}, {-32768, 335, 0},
        {32767, 335, 0}, {-1, -1, 1}
    };
    for (unsigned i = 0; i < sizeof(bounds) / sizeof(bounds[0]); i++) {
        setup();
        actor.range = bounds[i].range;
        actor.distance = bounds[i].distance;
        MineEmitterActor expected;
        memcpy(&expected, &actor, sizeof(expected));
        expected.timer = 9;
        expected.index = 1;
        func_8017E640(&actor);
        assert(memcmp(&actor, &expected, sizeof(actor)) == 0);
        if (bounds[i].spawn) {
            expect_order((unsigned[]){2, 4}, 2);
            assert(seen_subtype == 0x12 && seen_timer == 9 && seen_index == 0);
        } else {
            expect_order((unsigned[]){2}, 1);
            assert(seen_subtype == -1);
        }
    }

    setup();
    actor.subtype = 0;
    expired = 0;
    reset_on_refresh = 1;
    func_8017E640(&actor);
    expect_order((unsigned[]){1, 2}, 2);
    assert(actor.index == 1 && actor.timer == -19);

    setup();
    actor.index = 3;
    switch_on_ready = 1;
    expired = -9;
    allocate = 0;
    func_8017E640(&actor);
    expect_order((unsigned[]){2, 4}, 2);
    assert(seen_index == 0 && seen_subtype == 0x12 && seen_timer == 9 && actor.index == 1);

    for (int parity = 0; parity < 2; parity++) {
        setup();
        actor.subtype = 0;
        reset_on_refresh = 1;
        actor.range = 256;
        actor.distance = 336;
        sample = -12 + parity;
        switch_on_sample = change_on_spawn = 1;
        allocate = parity;
        func_8017E640(&actor);
        expect_order((unsigned[]){1, 2, 3, 4}, 4);
        assert(seen_index == 4 && seen_timer == 18 && seen_subtype == (parity ? 0x41 : 0x80));
        assert(actor.timer == -7913 && actor.index == -32768);
    }

    setup();
    actor.index = 3;
    func_8017E640(&actor);
    expect_order((unsigned[]){2, 4}, 2);
    assert(seen_subtype == 0x40 && actor.timer == 122 && actor.index == 4);
    return 0;
}
