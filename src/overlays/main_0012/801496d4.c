/* Owner re-derivation from retail assembly (clean input set). */
/* Returns 1 when the word at 0x178 is zero and bit 0x40 of the halfword at 0xAC is set, otherwise 0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xAC];
    u16 fAC;
    u8 padAE[0x178 - 0xAE];
    s32 f178;
} Obj801496D4;

s32 func_801496D4(Obj801496D4 *o) {
    s32 r;
    if (o->f178 != 0) {
        r = 0;
    } else {
        r = o->fAC & 0x40;
        if (r != 0) {
            r = 1;
        }
    }
    return r;
}
