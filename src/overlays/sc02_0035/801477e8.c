/* Owner re-derivation from retail assembly (clean input set). */
/* Stores v at +0x34 clamped to [-0x1F0000, 0x1F0000]. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x34];
    s32 v;
} Obj801477E8;

void func_801477E8(Obj801477E8 *o, s32 v) {
    o->v = v;
    if (v < -0x1F0000) {
        o->v = -0x1F0000;
    } else if (v > 0x1F0000) {
        o->v = 0x1F0000;
    }
}
