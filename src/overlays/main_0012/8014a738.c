/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
typedef struct {
    /* 0x000 */ u8 pad00[6];
    /* 0x006 */ u16 f06;
    /* 0x008 */ u8 pad08[6];
    /* 0x00E */ u16 f0E;
    /* 0x010 */ u8 pad10[0xC1 - 0x10];
    /* 0x0C1 */ u8 fC1;
    u8 pad_end[0x10C - 0xC2];
} Slot8014A738;
extern u8 D_801202A0[];
extern u8 func_80029178(s32 a);

s32 func_8014A738(u8 *o)
{
    s16 v[3];
    u32 i;
    Slot8014A738 *e;

    if (func_80029178(0x83) == 0) {
        return 0;
    }
    for (i = 0; i < 0x60; i++) {
        e = &((Slot8014A738 *)&D_801202A0)[i];
        if (e->fC1 != 7) {
            continue;
        }
        v[0] = *(u16 *)(o + 6) - e->f06;
        v[2] = *(u16 *)(o + 0xE) - e->f0E;
        if ((v[0] < 0 ? -v[0] : v[0]) < 0x40 && (v[2] < 0 ? -v[2] : v[2]) < 0x40) {
            return 1;
        }
    }
    return 0;
}
