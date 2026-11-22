/* Owner re-derivation of the scene floor probe. */
#include "../main_0012/floor_cell_map.h"

extern u32 D_8017F68C;
extern u32 D_8017F690;
extern u32 D_8017F694;
extern u32 D_8019F9B8;
extern u16 D_8019F9C8;

s32 func_80134510(SceneProbeVector *o)
{
    SceneProbeVector *b = FLOOR_ADDRESS(SceneProbeVector, D_8017F690);
    SceneProbeVector *a = FLOOR_ADDRESS(SceneProbeVector, D_8017F68C);
    int r = 0;
    u16 x;
    u16 y;
    u16 z;

    o->pad = 0;
    x = o->x;
    a->x = x;
    b->x = x;
    y = o->y;
    a->y = y - 4;
    b->y = y + 0x2FC;
    z = o->z;
    a->z = z;
    b->z = z;
    if (func_801345F8(FLOOR_ADDRESS(FloorCellMap, D_8019F9B8))) {
        s16 w;

        o->y = (FLOOR_ADDRESS(SceneProbeVector, D_8017F694))->y - 2;
        w = (FLOOR_ADDRESS(SceneProbeVector, D_8017F68C))->pad;
        if (w >= -0xBCB) {
            if (w < -0x578) {
                r = 0x4000;
            } else {
                r = 0x8000;
            }
        } else {
            r = 0x2000;
        }
        (FLOOR_ADDRESS(SceneProbeVector, D_8017F68C))->pad = D_8019F9C8;
    }
    return r;
}
