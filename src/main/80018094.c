/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Registers an object in a clamped-index table slot and, when flag 0x40000000 is set, re-registers it after a short setup sequence. */
#include "psx_types.h"

typedef struct {
    u32 table; /* guest u32 * */
    u8 pad[0x10];
} Bank;

extern Bank D_800A651C[];
extern u16 D_800B9A02;
extern void func_80058C6C(u32 *, s32);
extern s32 func_80010A08(s32);
extern s32 func_80058B40(s32, s32, s32, s32);
extern void func_8005A600(s32, s32, s32, s32, s32);

void func_80018094(u8 *obj, u32 idx, u32 flags) {
    s32 r;

    if (idx >= 0x1000) {
        idx = 0xFFF;
    }
    func_80058C6C((u32 *)D_800A651C[D_800B9A02].table + idx, (s32)obj);
    if (flags & 0x40000000) {
        obj[7] |= 2;
        r = func_80010A08(0xC);
        func_8005A600(r, 0, 0, func_80058B40(2, (flags >> 28) & 3, 0x280, 0) & 0xFFFF, 0);
        func_80058C6C((u32 *)D_800A651C[D_800B9A02].table + idx, r);
    }
}
