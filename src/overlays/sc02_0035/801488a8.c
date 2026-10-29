/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xA9];
    u8 kind;
    u16 hAA;
    u8 padAC[2];
    u16 hAE;
} O;

s32 func_801488A8(O *o) {
    u16 h;
    u32 lo;
    switch (o->kind) {
    case 0x41:
        return o->hAA & 0xF000;
    case 0x53:
    case 0x73:
        h = o->hAE;
        lo = h & 0xFF;
        if (lo != 0x80 || (h >> 8) != lo) {
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}