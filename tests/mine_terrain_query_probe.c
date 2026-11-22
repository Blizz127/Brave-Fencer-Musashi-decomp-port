#include "mine_terrain_query.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { PREVIOUS = 16, CURRENT = 24, GRID = 32, SECOND = 64 };
u32 D_8017F68C, D_8017F690, D_8019F9B8;
u16 D_8019F9C8;
s16 D_8019F9C4;

static FloorCellMap grids[2];
static SceneProbeVector previous[2], current[2];
static MineActionPoint from, to;
static struct {
    s16 mode, lower, upper, stop;
    unsigned hits, calls, distances;
    int rebind;
    s32 square, distance;
    SceneProbeVector near, far;
} trial;

void *bfm_floor_address(u32 address)
{
    switch (address) {
    case PREVIOUS: return &previous[0];
    case CURRENT: return &current[0];
    case GRID: return &grids[0];
    case PREVIOUS + SECOND: return &previous[1];
    case CURRENT + SECOND: return &current[1];
    case GRID + SECOND: return &grids[1];
    }
    abort();
}

static int equal_point(SceneProbeVector a, SceneProbeVector b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z && a.pad == b.pad;
}

s32 func_80047D3C(s32 squared)
{
    assert(trial.mode == 0 && trial.distances == 0 && trial.calls == 0);
    assert(squared == trial.square);
    assert(equal_point(previous[0], trial.near));
    assert(current[0].x == trial.far.x && current[0].z == trial.far.z);
    assert(current[0].y == 219 && current[0].pad == 32767);
    assert(D_8019F9C4 == 0 && D_8019F9C8 == 0);
    ++trial.distances;
    return trial.distance;
}

s32 func_80133AB0(s16 mode, s16 x, s16 z, FloorCellMap *grid)
{
    unsigned i = trial.calls;
    unsigned slot = trial.rebind && i != 0;
    assert(i < 6 && i <= trial.hits && mode == trial.mode);
    assert(grid == &grids[slot] && x == trial.near.x && z == trial.near.z);
    assert(equal_point(previous[slot], trial.near));
    assert(equal_point(current[slot], trial.far));
    assert(D_8019F9C4 == (i == 0 ? 0 : trial.stop));
    assert(D_8019F9C8 == (i == 0 ? 0 : 0x9200 + i));
    assert(trial.distances == (unsigned)(mode == 0));

    ++trial.calls;
    if (trial.rebind && i == 0) {
        D_8017F68C += SECOND;
        D_8017F690 += SECOND;
        D_8019F9B8 += SECOND;
        slot = 1;
    }
    s16 n = (s16)trial.calls;
    previous[slot] = trial.near = (SceneProbeVector){10+n, -20-n, 30+n, trial.lower};
    current[slot] = trial.far = (SceneProbeVector){-40-n, 50+n, -60-n, trial.upper};
    D_8019F9C4 = trial.stop;
    D_8019F9C8 = (u16)(0x9200 + n);
    return i < trial.hits;
}

static void reset(s16 mode)
{
    memset(&trial, 0, sizeof(trial));
    memset(previous, 0, sizeof(previous));
    memset(current, 0, sizeof(current));
    current[0].y = 219;
    D_8017F68C = PREVIOUS;
    D_8017F690 = CURRENT;
    D_8019F9B8 = GRID;
    D_8019F9C4 = -12;
    D_8019F9C8 = 0xcdef;
    from = (MineActionPoint){4, 260, 65535, {.flags = 0x54db}};
    to = (MineActionPoint){9, 31, 65532, {.flags = 0x8d31}};
    trial.mode = mode;
    trial.lower = -1600;
    trial.near = (SceneProbeVector){4, 260, -1, -32767};
    trial.far = (SceneProbeVector){9, 31, -4, 32767};
}

enum Output { UNCHANGED, FROM_FAR, FROM_NEAR };
static void check(unsigned n, u16 flags, enum Output output)
{
    MineActionPoint saved_from = from;
    MineActionPoint saved_to = to;
    assert(func_80133784(trial.mode, &from, &to) == flags);
    assert(trial.calls == n && trial.distances == (unsigned)(trial.mode == 0));
    assert(memcmp(&from, &saved_from, sizeof(from)) == 0);
    if (output == UNCHANGED) {
        assert(to.x == saved_to.x && to.y == saved_to.y && to.z == saved_to.z);
    } else {
        SceneProbeVector point = output == FROM_NEAR ? trial.near : trial.far;
        assert(to.x == (u16)point.x && to.y == (u16)point.y && to.z == (u16)point.z);
        if (output == FROM_NEAR) {
            /* The retry limit restores only the three coordinates. */
            unsigned slot = trial.rebind != 0;
            assert(current[slot].x == point.x && current[slot].y == point.y);
            assert(current[slot].z == point.z && current[slot].pad == trial.upper);
        }
    }
    assert(to.surface.flags == 0x9200+n);
}

static void normal_modes(void)
{
    reset(1);
    check(1, 0, UNCHANGED);

    static const struct { s16 lower, upper; u16 flags; } levels[] = {
        {-3020, -3019, 0}, {-3019, -3020, 0x6000}, {-1401, 0, 0x4000},
        {-1400, -3019, 0x8000}, {0, -3020, 0xa000}, {-3020, -3020, 0x2000}
    };
    for (unsigned i = 0; i < sizeof(levels)/sizeof(levels[0]); ++i) {
        reset(1);
        trial.stop = -1;
        trial.lower = levels[i].lower;
        trial.upper = levels[i].upper;
        check(1, levels[i].flags, FROM_FAR);
    }

    reset(-3);
    trial.hits = 1;
    trial.rebind = 1;
    check(2, 0x4000, FROM_FAR);

    reset(1);
    trial.hits = 5;
    check(6, 0x4000, FROM_FAR);

    reset(1);
    trial.hits = 6;
    trial.rebind = 1;
    check(6, 0x2000, FROM_NEAR);

    reset(2);
    from.y = 32765;
    trial.near.y = 32765;
    trial.far = (SceneProbeVector){4, -32765, -1, 32767};
    trial.hits = 6;
    check(1, 0x4000, FROM_FAR);
}

static void distance_mode(void)
{
    static const struct { s32 supplied; s16 y; } heights[] = {
        {-1, 36}, {0, 36}, {2, 36}, {3, 36}, {4, 37}, {5, 37}, {17, 49},
        /* The positive endpoint is 31. Only its resulting low halfword is stored. */
        {INT_MAX, 31}
    };
    for (unsigned i = 0; i < sizeof(heights)/sizeof(heights[0]); ++i) {
        reset(0);
        trial.near.y = 256;
        trial.square = 34;
        trial.distance = heights[i].supplied;
        trial.far.y = heights[i].y;
        check(1, 0, UNCHANGED);
    }

    reset(0);
    from.x = to.x = from.z = to.z = 0;
    trial.near = (SceneProbeVector){0, 256, 0, -32767};
    trial.far = (SceneProbeVector){0, 36, 0, 32767};
    trial.hits = 1;
    check(2, 0x4000, FROM_FAR);

    reset(0);
    to = from;
    trial.near.y = 256;
    trial.far = (SceneProbeVector){4, 265, -1, 32767};
    trial.hits = 6;
    check(1, 0x4000, FROM_FAR);

    reset(0);
    from.x = from.z = 0; from.y = 0x8001;
    to.x = to.z = 0x8000; to.y = 32766;
    trial.near = (SceneProbeVector){0, 32765, 0, -32767};
    trial.far = (SceneProbeVector){-32768, -32764, -32768, 32767};
    trial.square = (-2147483647 - 1);
    trial.distance = 5;
    check(1, 0, UNCHANGED);
}

int main(void)
{
    normal_modes();
    distance_mode();
    return 0;
}
