/* Exercise the grid's two face streams and the material selected on return. */
#include <assert.h>
#include "forest_floor.h"

static FloorPoint near_point, far_point, movement, vertices[2];
FloorPoint *D_801870AC = &near_point, *D_801870B0 = &far_point, *D_801870B8 = &movement;
s16 D_801D94FC;
u16 D_801D9500;
static s32 distances[2];
static u16 short_faces[18], long_faces[22];
static int calls, outcome, auxiliary;

s16 func_80133CD4(s16 flags, FloorFace *face, FloorPoint *points, s32 *distance)
{
    assert(points == vertices && distance == distances);
    if (++calls == 1) {
        assert(flags == 9 && (void *)face == (void *)(short_faces + 9));
        assert(face->flags == 0x1234);
        return 0;
    }
    assert(calls == 2 && (u16)flags == 0x8009);
    assert((void *)face == (void *)(long_faces + 11));
    assert(face->flags == 0x3456);
    face->flags = 0x789a;
    D_801D94FC = auxiliary;
    return outcome;
}

int main(void)
{
    u16 cells[8] = {0, 0, 0, 0, 0, 0, 2, 2};
    u16 tokens[3] = {0, 1, 0x8001};
    FloorGrid grid = {255, 255, 2, 2, cells, distances, vertices, (u8 *)tokens,
                      (u8 *)short_faces, (u8 *)long_faces};
    near_point = (FloorPoint){10, 65530, 32, 100};
    far_point = (FloorPoint){3, 8, 32, -50};
    movement.w = 222;
    short_faces[9] = 0x1234;
    for (int mode=0; mode<3; ++mode) {
        calls = 0;
        outcome = mode == 0 ? 1 : mode == 1 ? -1 : 0;
        auxiliary = mode == 2;
        D_801D94FC = 0;
        D_801D9500 = 0x2222;
        long_faces[11] = 0x3456;
        assert(func_80133AB0(9, 0, 0, &grid) == (mode != 2));
        assert(calls == 2);
        assert(D_801D9500 == (mode == 1 ? 0x2222 : 0x789a));
        assert((s16)movement.x == -7 && movement.y == 14 && movement.z == 0 && movement.w == 222);
    }
    calls = 0;
    assert(func_80133AB0(9, -32768, 0, &grid) == 0 && calls == 0);
    return 0;
}
