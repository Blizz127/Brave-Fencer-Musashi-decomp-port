/* Exercise the two-cell search with a grid callback that can change the live scratch pointers. */
#include "forest_floor.h"
#include <assert.h>

FloorPoint *D_801870AC, *D_801870B0, *D_801870B4, *D_801870B8;
FloorGrid *D_801D94F0;
u16 D_801D9500;

static FloorPoint near_point, far_point, projected, motion;
static FloorPoint next_near = {127, 30, 42, 0}, next_far = {128, 31, 42, 0};
static FloorPoint next_hit = {420, (u16)-30, 32, 0};
static FloorGrid first_grid, next_grid;
static unsigned calls, success, rebind;
static s16 sample_x[2], sample_z[2];
static u16 seen_mode[2];

s32 func_80134A74(s32 mode, s16 x, s16 z, FloorGrid *grid)
{
    unsigned i = calls++;
    assert(i < 2 && mode >= 0 && mode <= 65535);
    assert(grid == (i && rebind ? &next_grid : &first_grid));
    assert(D_801D9500 == (i ? 0x3412 : 0));
    seen_mode[i] = (u16)mode;
    sample_x[i] = x;
    sample_z[i] = z;
    D_801D9500 = i ? 0x0280 : 0x3412;
    if (!i && rebind) {
        D_801870AC = &next_near;
        D_801870B0 = &next_far;
        D_801870B4 = &next_hit;
        D_801D94F0 = &next_grid;
    }
    return success == i + 1 ? -1 : 0;
}

static void reset(unsigned accepted, unsigned change_owner)
{
    calls = 0;
    success = accepted;
    rebind = change_owner;
    D_801870AC = &near_point;
    D_801870B0 = &far_point;
    D_801870B4 = &projected;
    D_801870B8 = &motion;
    D_801D94F0 = &first_grid;
    D_801D9500 = 0xaaaa;
}

int main(void)
{
    FloorPoint start = {0, (u16)-200, (u16)-120, 0xbeef};
    FloorPoint end = {127, 20, (u16)-1, 0x4242};
    reset(0, 0);
    assert(!func_80135004(0x8011, &start, &end));
    assert(calls == 1 && sample_x[0] == 0 && sample_z[0] == -120 && seen_mode[0] == 0x8011);
    assert((s16)motion.x == 127 && (s16)motion.y == 220 && (s16)motion.z == 119);
    assert(end.x == 127 && end.z == (u16)-1 && end.w == 0x4242);

    reset(2, 1);
    assert(func_80135004(0x8011, &start, &end) == 1);
    assert(calls == 2 && sample_x[1] == 128 && sample_z[1] == 42 && seen_mode[1] == 0x8011);
    assert(end.x == next_hit.x && end.y == next_hit.y && end.z == next_hit.z);
    assert((u16)end.w == 0x0280 && D_801D9500 == 0x0280);

    reset(1, 0);
    projected = (FloorPoint){20, 40, 60, 0};
    assert(func_80135004(0, &start, &projected) == 1);
    assert(calls == 1 && projected.x == 20 && projected.y == 40 && projected.z == 60);
    assert((u16)projected.w == 0x3412);

    reset(0, 0);
    end = (FloorPoint){0, 20, 0, 0x6262};
    assert(!func_80135004(2, &start, &end));
    assert(calls == 2 && sample_x[1] == 0 && sample_z[1] == 0);
    assert(end.x == 0 && end.y == 20 && end.z == 0 && end.w == 0x6262);
    return 0;
}
