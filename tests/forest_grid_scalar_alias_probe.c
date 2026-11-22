/* A model point can cover the two scene contact fields, which are four bytes apart. */
#include <assert.h>
#include "forest_floor.h"

static union {
    FloorPoint point;
    struct { s16 latch; u16 pad, material, tail; } fields;
} work;
static FloorPoint far_point;
static FloorGrid grid;
FloorPoint *D_801870AC = &work.point, *D_801870B0 = &far_point;
FloorGrid *D_801D94F0 = &grid;

#define D_801D94FC (work.fields.latch)
#define D_801D9500 (work.fields.material)
#include "../src/overlays/sc01_0077/801343c4.c"
#undef D_801D94FC
#undef D_801D9500

static int calls;
s32 func_80133AB0(s16 mode, s16 x, s16 z, FloorGrid *map)
{
    assert(++calls == 1 && mode == 1 && map == &grid);
    assert(x == -400 && z == 17);
    assert(work.fields.latch == 0 && work.fields.material == 0);
    work.fields.material = 0x4567;
    return 1;
}

int main(void)
{
    FloorPoint start = {(u16)-400, 8, 17, 0}, end = {63, 6, 7, 0};
    assert(func_801343C4(1, &start, &end) == 1);
    assert(calls == 1 && (u16)end.w == 0x4567);
    return 0;
}
