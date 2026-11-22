#include "mine_terrain_query.h"

#define PREVIOUS_POINT FLOOR_ADDRESS(SceneProbeVector, D_8017F68C)
#define CURRENT_POINT FLOOR_ADDRESS(SceneProbeVector, D_8017F690)

s32 func_80133784(s16 mode, MineActionPoint *from, MineActionPoint *to)
{
    u16 flags = 0;
    s16 tries = 0;
    s16 hit = 0;
    s16 once = 0;
    s16 dx, dy, dz;
    s32 result;

    PREVIOUS_POINT->pad = -32767;
    CURRENT_POINT->pad = 32767;
    PREVIOUS_POINT->x = from->x;
    PREVIOUS_POINT->z = from->z;
    CURRENT_POINT->x = to->x;
    CURRENT_POINT->z = to->z;
    D_8019F9C8 = 0;
    D_8019F9C4 = 0;

    if (mode == 0) {
        dx = to->x - from->x;
        dy = to->y - from->y;
        dz = to->z - from->z;
        if (dx == 0 && dy == 0 && dz == 0)
            once = 1;
        PREVIOUS_POINT->y = from->y - 4;
        result = func_80047D3C((u32)(dx * dx) + (u32)(dz * dz));
        if (result < 3)
            result = 4;
        else if (result < 5)
            result++;
        CURRENT_POINT->y = to->y + (u32)result + 1;
    } else {
        PREVIOUS_POINT->y = from->y;
        if (mode == 2) {
            CURRENT_POINT->x = PREVIOUS_POINT->x;
            CURRENT_POINT->y = PREVIOUS_POINT->y + 6;
            CURRENT_POINT->z = PREVIOUS_POINT->z;
            once = 1;
        } else {
            CURRENT_POINT->y = to->y;
        }
    }

    for (;;) {
        result = func_80133AB0(mode, PREVIOUS_POINT->x, PREVIOUS_POINT->z,
                               FLOOR_ADDRESS(FloorCellMap, D_8019F9B8));
        if (result == 0)
            break;
        hit |= result;
        if (once)
            break;
        if (tries++ >= 5) {
            CURRENT_POINT->x = PREVIOUS_POINT->x;
            CURRENT_POINT->y = PREVIOUS_POINT->y;
            CURRENT_POINT->z = PREVIOUS_POINT->z;
            flags = 0x2000;
            goto corrected;
        }
    }

    if (hit || D_8019F9C4) {
        s16 lower = PREVIOUS_POINT->pad;
        if (lower >= -3019) {
            if (lower < -1400)
                flags |= 0x4000;
            else
                flags |= 0x8000;
        }
        if (CURRENT_POINT->pad < -3019)
            flags |= 0x2000;
corrected:
        to->x = CURRENT_POINT->x;
        to->y = CURRENT_POINT->y;
        to->z = CURRENT_POINT->z;
        to->surface.flags = D_8019F9C8;
        return flags;
    }
    to->surface.flags = D_8019F9C8;
    return 0;
}
