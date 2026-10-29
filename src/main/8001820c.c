/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Inserts a primitive into the current ordering table at a clamped depth, bracketed by two newly allocated draw-mode packets (optionally marking it semi-transparent). */
#include "psx_types.h"

typedef struct {
    u32 ot; /* guest s32 * */
    u8 pad[0x14 - 4];
} OtBuffer;

extern void *func_80010A08(s32);
extern u32 func_80058B40(s32, s32, s32, s32);
extern void func_80058C6C(void *, void *);
extern void func_8005A600(void *, s32, s32, s32, s32);
extern OtBuffer D_800A651C[];
extern u16 D_800B9A02;

void func_8001820C(u32 *prim, u32 depth, u32 flags) {
    void *mode;
    s32 blend;

    if (flags & 0x40000000) {
        prim[1] |= 0x02000000;
    }
    if (depth >= 0x1000) {
        depth = 0xFFF;
    }
    mode = func_80010A08(0xC);
    blend = (flags >> 28) & 3;
    func_8005A600(mode, 0, 0, func_80058B40(2, blend, 0x280, 0) & 0xFFFF, 0);
    func_80058C6C((s32 *)D_800A651C[D_800B9A02].ot + depth, mode);
    func_80058C6C((s32 *)D_800A651C[D_800B9A02].ot + depth, prim);
    mode = func_80010A08(0xC);
    func_8005A600(mode, 0, 1, func_80058B40(2, blend, 0x280, 0) & 0xFFFF, 0);
    func_80058C6C((s32 *)D_800A651C[D_800B9A02].ot + depth, mode);
}
