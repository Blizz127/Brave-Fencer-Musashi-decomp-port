/* Supply a controlled contact while exercising the real coordinate gather and scatter. */
#include "forest_floor.h"
#include <assert.h>
#include <string.h>

FloorPoint D_801152A8;
static FloorPoint saved = {30, 40, 50, 60};
static FloorPoint current = {91, 92, 93, 701};
static FloorPoint projected = {801, 802, 803, 804};
static s16 normal_y;
static int hit;

s32 func_80135004(u16 flags, FloorPoint *from, FloorPoint *to)
{
    assert(flags == 1 && from != to);
    assert(!memcmp(from, &saved, sizeof saved));
    assert(!memcmp(to, &current, sizeof current));
    *from = (FloorPoint){0, 0, 0, 0};
    if (hit) {
        *to = projected;
        D_801152A8.y = (u16)normal_y;
    }
    return hit;
}

static s32 sweep_result;

s32 func_80133784(s16 mode, FloorPoint *from, FloorPoint *to)
{
    assert(mode == 1 && !memcmp(from, &saved, sizeof saved));
    assert(!memcmp(to, &current, sizeof current));
    if (sweep_result) *to = projected;
    return sweep_result;
}

static void check(int accepted, s16 y, s32 slope, int model_grid)
{
    /* The actor stores its three current position halfwords four bytes apart. */
    u16 actor[48], expected[48];
    FloorPoint end = {0, 0, 0, current.w};
    for (unsigned i = 0; i < 48; ++i) actor[i] = 0x55aa;
    actor[3] = current.x;
    actor[5] = current.y;
    actor[7] = current.z;
    memcpy(actor + 0x22, &saved, sizeof saved);
    memcpy(expected, actor, sizeof actor);
    hit = accepted;
    normal_y = y;
    if (model_grid) {
        sweep_result = slope;
        assert(func_8016380C((u8 *)actor, &end) == slope);
    } else {
        assert(func_801638A0((u8 *)actor, &end) == slope);
    }
    if (accepted) {
        expected[3] = projected.x;
        expected[5] = projected.y;
        expected[7] = projected.z;
    }
    assert(!memcmp(&end, accepted ? &projected : &current, sizeof end));
    assert(!memcmp(actor, expected, sizeof actor));
}

int main(void)
{
    check(0, 0, 0, 0);
    check(1, -32768, 0x2000, 0);
    check(1, -3020, 0x2000, 0);
    check(1, -3019, 0x4000, 0);
    check(1, -1401, 0x4000, 0);
    check(1, -1400, 0x8000, 0);
    check(1, 32767, 0x8000, 0);
    check(0, 0, 0, 1);
    check(1, 0, 0x2000, 1);
    check(1, 0, 0x8000, 1);
    check(1, 0, 0x12348000, 1);
    return 0;
}
