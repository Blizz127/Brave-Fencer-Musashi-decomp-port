/* Owner re-derivation from retail assembly (clean input set). */
/* For each of 8 flagged slots of a record, resets a companion record's motion fields. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x20];
    s32 f20;
    u8 pad24[4];
    s32 f28;
    s32 f2C;
    s16 f30;
    u8 pad32[5];
    u8 f37;
    u8 f38;
    u8 pad39[0x54 - 0x39];
} Slot;

extern u8 D_800A4694[];

void func_8002F150(s32 arg0) {
    u8 *src = D_800A4694 + arg0 * 84;
    Slot *dst = (Slot *)(D_800A4694 + 0x2F4);
    s32 i;
    for (i = 0; i < 8; i++) {
        if ((src + i)[0xE] != 0) {
            Slot *d = &dst[i];
            d->f30 = 0;
            d->f38 = 0;
            d->f2C = -0x18;
            d->f28 = d->f20 - 0x100;
            d->f37 |= 1;
        }
    }
}
