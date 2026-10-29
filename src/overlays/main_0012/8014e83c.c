/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
typedef struct {
    /* 0x000 */ u16 id;
    /* 0x002 */ u8 pad02[4];
    /* 0x006 */ u16 f06;
    /* 0x008 */ u8 pad08[2];
    /* 0x00A */ u16 f0A;
    /* 0x00C */ u8 pad0C[2];
    /* 0x00E */ u16 f0E;
    /* 0x010 */ u8 pad10[0x10];
    /* 0x020 */ u32 f20;
    /* 0x024 */ u8 pad24[0x34];
    /* 0x058 */ u32 f58;
    /* 0x05C */ u16 f5C;
    u8 pad_end[0x10C - 0x5E];
} Slot8014E83C;
extern u8 D_801202A0[];
extern s32 func_80135888(u32 obj, u32 flags, u32 from, u32 to);

s32 func_8014E83C(u8 *o, u32 a, u32 b)
{
    Slot8014E83C *e;

    for (e = ((Slot8014E83C *)&D_801202A0); e < &((Slot8014E83C *)&D_801202A0)[96]; e++) {
        if (e->id != 0 && (e->f5C & 0x20) != 0 && e->f58 != 0
            && func_80135888(e->f20, e->f58, a, b) != 0) {
            *(Slot8014E83C **)(o + 0x180) = e;
            *(u16 *)(o + 6) = e->f06;
            *(u16 *)(o + 0xA) = e->f0A;
            *(u16 *)(o + 0xE) = e->f0E;
            return 1;
        }
    }
    return 0;
}
