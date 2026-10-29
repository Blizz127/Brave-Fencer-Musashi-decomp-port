/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Links a primitive into the current ordering table at a clamped depth, optionally marking it semi-transparent. */
#include "psx_types.h"

typedef struct {
    u32 ot; /* guest s32 * */
    u8 pad[0x14 - 4];
} OtBuffer;

extern void func_80058C6C(void *, void *);
extern OtBuffer D_800A651C[];
extern u16 D_800B9A02;

void func_80018194(u32 *prim, u32 depth, u32 flags) {
    if (flags & 0x40000000) {
        prim[1] |= 0x02000000;
    }
    if (depth >= 0x1000) {
        depth = 0xFFF;
    }
    func_80058C6C((s32 *)D_800A651C[D_800B9A02].ot + depth, prim);
}
