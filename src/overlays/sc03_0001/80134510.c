#include "forest_floor.h"

/* SC03/0001's live probe, intersection, selected map and accepted material. */
extern FloorPoint *D_8018DA9C;
extern FloorPoint *D_8018DAA0;
extern FloorPoint *D_8018DAA4;
extern FloorGrid *D_801ED3B0;
extern u16 D_801ED3C0;

u32 func_80134510(FloorPoint *point)
{
    FloorPoint *end = D_8018DAA0;
    FloorPoint *start = D_8018DA9C;
    u32 slope;
    u16 x = point->x, y, z;

    point->w = 0;
    start->x = x;
    end->x = x;
    y = point->y;
    start->y = y - 4;
    end->y = y + 764;
    z = point->z;
    start->z = z;
    end->z = z;
    slope = 0;
    if (func_801345F8(D_801ED3B0)) {
        s32 normal_y;
        point->y = D_8018DAA4->y - 2;
        normal_y = D_8018DA9C->w;
        if (normal_y >= -3019) {
            if (normal_y < -1400)
                slope = 0x4000;
            else
                slope = 0x8000;
        } else {
            slope = 0x2000;
        }
        D_8018DA9C->w = D_801ED3C0;
    }
    return slope;
}
