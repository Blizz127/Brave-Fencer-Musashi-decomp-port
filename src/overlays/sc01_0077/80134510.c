/* Set the vertical probe ends, then adjust the actor height and report the slope class on a hit. */
#include "forest_floor.h"

u32 func_80134510(FloorPoint *point)
{
    u32 slope;
    FloorPoint *far_point = D_801870B0;
    FloorPoint *near_point = D_801870AC;

    u16 x = point->x;
    u16 y;
    u16 z;

    point->w = 0;
    near_point->x = x;
    far_point->x = x;
    y = point->y;
    near_point->y = y - 4;
    far_point->y = y + 0x2FC;
    z = point->z;
    near_point->z = z;
    far_point->z = z;
    slope = 0;
    if (func_801345F8(D_801D94F0, near_point, far_point)) {
        s32 normal_y;
        point->y = D_801870B4->y - 2;
        normal_y = D_801870AC->w;
        if (normal_y >= -0xBCB) {
            if (normal_y < -0x578) {
                slope = 0x4000;
            } else {
                slope = 0x8000;
            }
        } else {
            slope = 0x2000;
        }
        D_801870AC->w = D_801D9500;
    }
    return slope;
}
