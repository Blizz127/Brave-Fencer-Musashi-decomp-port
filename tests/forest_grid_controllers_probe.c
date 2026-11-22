/* Isolate the two scene controllers at their grid and square-root callbacks. */
#include <assert.h>
#include "forest_floor.h"

static FloorPoint near_point, far_point;
static FloorGrid first_grid, second_grid;
FloorPoint *D_801870AC = &near_point, *D_801870B0 = &far_point;
FloorGrid *D_801D94F0 = &first_grid;
s16 D_801D94FC;
u16 D_801D9500;
static int calls, kind, square_roots;

s32 func_80047D3C(s32 squared)
{
    assert(kind == 3 && squared == 0);
    ++square_roots;
    return 0;
}

s32 func_80133AB0(s16 flags, s16 x, s16 z, FloorGrid *grid)
{
    ++calls;
    if (kind == 0) {
        assert(flags == -4);
        if (calls == 1) {
            assert(x == 127 && z == 25 && grid == &first_grid);
            D_801D94F0 = &second_grid;
            return 0;
        }
        assert(calls == 2 && x == 128 && z == 25 && grid == &second_grid);
        far_point.y = 81;
        D_801D9500 = 0xbeef;
        return 1;
    }
    assert(grid == &first_grid);
    if (kind == 1) {
        assert(calls == 1 && flags == -4);
        return 0;
    }
    assert(calls == 1);
    if (kind == 2) {
        assert(flags == 2 && near_point.y == 10 && far_point.y == 16);
        assert(near_point.x == far_point.x && near_point.z == far_point.z);
        near_point.w = -3019;
        D_801D9500 = 0x1234;
        return 1;
    }
    assert(flags == 0 && square_roots == 1);
    assert(near_point.y == 6 && far_point.y == 15);
    return 0;
}

int main(void)
{
    FloorPoint start = {127, 10, 25, 42};
    FloorPoint end = {128, 11, 25, -11};
    kind = 0;
    assert(func_801343C4(-4, &start, &end) == 1);
    assert(calls == 2 && end.y == 81 && (u16)end.w == 0xbeef);

    kind = 1;
    calls = 0;
    D_801D94F0 = &first_grid;
    end = (FloorPoint){126, 11, 25, -11};
    assert(func_801343C4(-4, &start, &end) == 0);
    assert(calls == 1 && end.y == 11 && end.w == -11);

    kind = 2;
    calls = 0;
    assert(func_80133784(2, &start, &end) == 0x4000);
    assert(end.w == 0x1234 && end.x == 127 && end.y == 16 && square_roots == 0);

    kind = 3;
    calls = 0;
    end = start;
    assert(func_80133784(0, &start, &end) == 0);
    assert(end.w == 0 && square_roots == 1);
    return 0;
}
