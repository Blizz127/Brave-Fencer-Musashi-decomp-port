/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Inserts a byte-addressed primitive into the current ordering table at a clamped depth, bracketed by two new draw-mode packets (optionally setting its semi-transparency bit). */
#include "primitive_ordering_table.h"

void func_80017F14(void *prim, u32 depth, u32 flags) {
    void *mode;
    s32 blend;

    if (depth >= 0x1000) {
        depth = 0xFFF;
    }
    mode = func_80010A08(0xC);
    blend = (flags >> 28) & 3;
    func_8005A600(mode, 0, 0, func_80058B40(2, blend, 0x280, 0) & 0xFFFF, 0);
    func_80058C6C((u32 *)D_800A651C[D_800B9A02].table + depth, mode);
    func_80058C6C((u32 *)D_800A651C[D_800B9A02].table + depth, prim);
    if (flags & 0x40000000) {
        ((u8 *)prim)[7] |= 2;
    }
    mode = func_80010A08(0xC);
    func_8005A600(mode, 0, 1, func_80058B40(2, blend, 0x280, 0) & 0xFFFF, 0);
    func_80058C6C((u32 *)D_800A651C[D_800B9A02].table + depth, mode);
}
