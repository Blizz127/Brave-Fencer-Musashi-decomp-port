/* Sweep the two working endpoints against the scene's model grid. */
#include "forest_floor.h"

extern s32 func_80047D3C(s32 squared);

#ifdef __mips__
typedef FloorPoint SweepPoint;
#else
/* Input, output and scene work points may overlap one halfword apart. */
typedef volatile FloorPoint SweepPoint;
#endif
#define SWEEP_POINT(address) ((SweepPoint *)(address))

s32 func_80133784(s16 mode, FloorPoint *first, FloorPoint *last)
{
    SweepPoint *point;
    s32 flags;
    s16 done;
    s16 count;
    s16 hits;
    s32 result;
    s16 dx, dy, dz;

    flags = 0;
    count = 0;
    hits = 0;
    SWEEP_POINT(D_801870AC)->w = -32767;
    SWEEP_POINT(D_801870B0)->w = 32767;
    SWEEP_POINT(D_801870AC)->x = SWEEP_POINT(first)->x;
    SWEEP_POINT(D_801870AC)->z = SWEEP_POINT(first)->z;
    done = 0;
    SWEEP_POINT(D_801870B0)->x = SWEEP_POINT(last)->x;
    D_801D9500 = 0;
    D_801D94FC = 0;
    SWEEP_POINT(D_801870B0)->z = SWEEP_POINT(last)->z;
    if (mode == 0) {
        dx = SWEEP_POINT(last)->x - SWEEP_POINT(first)->x;
        dy = SWEEP_POINT(last)->y - SWEEP_POINT(first)->y;
        dz = SWEEP_POINT(last)->z - SWEEP_POINT(first)->z;
        if (dx == 0 && dy == 0)
            done = dz == 0;
        SWEEP_POINT(D_801870AC)->y = SWEEP_POINT(first)->y - 4;
        result = func_80047D3C((s32)((u32)(dx * dx) + (u32)(dz * dz)));
        if (result < 3)
            result = 4;
        else if (result < 5)
            result++;
        SWEEP_POINT(D_801870B0)->y = (u32)SWEEP_POINT(last)->y + (u32)result + 1;
    } else {
        SWEEP_POINT(D_801870AC)->y = SWEEP_POINT(first)->y;
        if (mode == 2) {
            SWEEP_POINT(D_801870B0)->x = SWEEP_POINT(D_801870AC)->x;
            SWEEP_POINT(D_801870B0)->y = SWEEP_POINT(D_801870AC)->y + 6;
            SWEEP_POINT(D_801870B0)->z = SWEEP_POINT(D_801870AC)->z;
            done = 1;
        } else {
            SWEEP_POINT(D_801870B0)->y = SWEEP_POINT(last)->y;
        }
    }
    for (;;) {
        result = func_80133AB0(mode, SWEEP_POINT(D_801870AC)->x,
                               SWEEP_POINT(D_801870AC)->z, D_801D94F0);
        if (result == 0)
            break;
        hits |= result;
        if (done != 0)
            break;
        if (count++ < 5)
            continue;
        SWEEP_POINT(D_801870B0)->x = SWEEP_POINT(D_801870AC)->x;
        SWEEP_POINT(D_801870B0)->y = SWEEP_POINT(D_801870AC)->y;
        SWEEP_POINT(D_801870B0)->z = SWEEP_POINT(D_801870AC)->z;
        flags = 0x2000;
        goto finish;
    }
    if (hits != 0 || D_801D94FC != 0) {
        if (SWEEP_POINT(D_801870AC)->w >= -3019) {
            if (SWEEP_POINT(D_801870AC)->w < -1400)
                flags |= 0x4000;
            else
                flags |= 0x8000;
        }
        if (SWEEP_POINT(D_801870B0)->w < -3019)
            flags |= 0x2000;
finish:
        point = D_801870B0;
        SWEEP_POINT(last)->x = point->x;
        SWEEP_POINT(last)->y = point->y;
        SWEEP_POINT(last)->z = point->z;
        SWEEP_POINT(last)->w = (s16)D_801D9500;
        return (u16)flags;
    }
    SWEEP_POINT(last)->w = (s16)D_801D9500;
    return 0;
}
