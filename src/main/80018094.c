/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Registers an object in a clamped-index table slot and, when flag 0x40000000 is set, re-registers it after a short setup sequence. */
#include "primitive_ordering_table.h"

void func_80018094(void *obj, u32 idx, u32 flags) {
    void *mode;

    if (idx >= 0x1000) {
        idx = 0xFFF;
    }
    func_80058C6C((u32 *)D_800A651C[D_800B9A02].table + idx, obj);
    if (flags & 0x40000000) {
        ((u8 *)obj)[7] |= 2;
        mode = func_80010A08(0xC);
        func_8005A600(mode, 0, 0, func_80058B40(2, (flags >> 28) & 3, 0x280, 0) & 0xFFFF, 0);
        func_80058C6C((u32 *)D_800A651C[D_800B9A02].table + idx, mode);
    }
}
